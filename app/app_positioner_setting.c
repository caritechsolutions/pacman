#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_pop.h"
#include "module/app_frontend.h"
#include "app_epg.h"
#include "app_dvbs_sat_opt.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

extern void GxFrontend_StartMonitor(int32_t tuner);
extern void GxFrontend_StopMonitor(int32_t tuner);

#define CMBOX_POSITIONER_TUNER "cmbox_positioner_tuner"

#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus"
#define IMG_BAR_HALF_FOCUS_L "s_bar_half_focus_l"

#define IMG_BAR_HALF_FOCUS_AR "s_bar_half_focus_arrow"
#define IMG_BAR_HALF_UNFOCUS_AR "s_bar_half_unfocus_arrow"
#define IMG_BUTTON_FOCUS_OK "s_button_ok"

#define MAX_POSITIONER_OFF_ITEM 4
#define MAX_POSITIONER_ON_ITEM 8

#define DISEQC12_LIMIT_CONTENT "[E,W]"
#define MAX_DISEQC12_POS 64

typedef enum
{
	MOTOR_BUTTON_OK_HIDE=0,
	MOTOR_BUTTON_OK_SHOW
}PositionerStatus;

static enum
{
	POSITIONER_OFF,
	POSITIONER_DISEQC12,
	POSITIONER_USALS
}s_positioner_mode = POSITIONER_OFF;

static char *s_positioner_item_widget[][MAX_POSITIONER_ON_ITEM] =
{
	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"btn_positioner_off_save",
	},

	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"cmb_positioner_fine_drive",
		"cmb_positioner_limit",
		"btn_positioner_recalc",
		"cmb_positioner_diseqc12_goto",
		"btn_positioner_save"
	},

	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"box_positioner_longitude",
		"box_positioner_latitude",
		"btn_positioner_longitude",
		"btn_positioner_usals_goto",
		"btn_positioner_save"
	}
};

static char *s_positioner_title_content[][MAX_POSITIONER_ON_ITEM] =
{
	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_ID_SAVE,
	},

	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_ID_FINE_DRIVE,
		STR_ID_LIMIT,
		STR_ID_RECALC,
		STR_ID_GOTO,
		STR_ID_SAVE_POSITION
	},

	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_ID_LOCAL_LONGITUDE,
		STR_ID_LOCAL_LATITUDE,
		STR_ID_SAT_LONGITUDE,
		STR_ID_GOTO,
		STR_ID_SAVE
	}
};

static char *s_positioner_item_title[] =
{
	"text_positioner_item_title1",
	"text_positioner_item_title2",
	"text_positioner_item_title3",
	"text_positioner_item_title4",
	"text_positioner_item_title5",
	"text_positioner_item_title6",
	"text_positioner_item_title7",
	"text_positioner_item_title8",
};

static char *s_positioner_choice_back[] =
{
	"img_positioner_item_choice1",
	"img_positioner_item_choice2",
	"img_positioner_item_choice3",
	"img_positioner_item_choice4",
	"img_positioner_item_choice5",
	"img_positioner_item_choice6",
	"img_positioner_item_choice7",
	"img_positioner_item_choice8",
};

#if UI_MIRROR_SUPPORT
static char *s_positioner_ok_choice_tag[] =
{
	"img_positioner_item_ok_choice1",
	"img_positioner_item_ok_choice2",
	"img_positioner_item_ok_choice3",
	"img_positioner_item_ok_choice4",
    "img_positioner_item_ok_choice5",
};
#endif

static int s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
static int s_positioner_sel = 0;
static uint8_t s_diseqc12_pos_map[MAX_DISEQC12_POS] = {0};
static event_list* sp_PosiTimerSignal = NULL;
static event_list* sp_PosiLockTimer = NULL;
static event_list* sp_PosiArrow = NULL;
static LongitudeDirect s_local_longitude_direct;
static LatitudeDirect s_local_latitude_direct;
static char* s_moto_type_content[] =
{
	"Off",
	"DiSEqC1.2",
	"USALS"
};
static enum
{
	DISH_MOVE_NONE,
	DISH_MOVE_EAST,
	DISH_MOVE_WEST
}s_dish_moving_direct = DISH_MOVE_NONE;

static void _positioner_sat_position_get(PositionVal *sat_position)
{
	char *str = NULL;
	sat_position->longitude.longitude_direct = s_local_longitude_direct;
	GUI_GetProperty("edit_positioner_longitude", "string", &str);
	sat_position->longitude.longitude = 10 * app_float_edit_str_to_value(str);

	sat_position->latitude.latitude_direct = s_local_latitude_direct;
	GUI_GetProperty("edit_positioner_latitude", "string", &str);
	sat_position->latitude.latitude = 10 * app_float_edit_str_to_value(str);

	return;
}

static int app_positioner_clean_signal(void)
{
	app_clean_signal("progbar_positioner_strength", "text_positioner_strength_value",
						"progbar_positioner_quality", "text_positioner_quality_value");

	return 0;
}

static int timer_positioner_show_signal(void *userdata)
{
	app_show_signal("progbar_positioner_strength", "text_positioner_strength_value",
						"progbar_positioner_quality", "text_positioner_quality_value");

	return 0;
}

static void timer_positioner_show_signal_stop(void)
{
	if(sp_PosiTimerSignal != NULL)
		timer_stop(sp_PosiTimerSignal);
}

static void timer_positioner_show_signal_reset(void)
{
	if(sp_PosiTimerSignal != NULL)
		reset_timer(sp_PosiTimerSignal);
}

static int timer_positioner_set_frontend(void *userdata)
{
	PositionVal sat_position;

    sp_PosiLockTimer = NULL;
    _positioner_sat_position_get(&sat_position);
	s_dvbs_sat_data.set_diseqc(&sat_position);
	s_dvbs_sat_data.set_tp(SET_TP_FORCE);

	return 0;
}

static int timer_positioner_stop_fine_drive(void *userdata)
{
	APP_TIMER_REMOVE(sp_PosiArrow);

	app_diseqc12_stop_drictory(s_dvbs_sat_data.cur_sat.sat_data.tuner,3);	//X2
    GxFrontend_StartMonitor(s_dvbs_sat_data.cur_sat.sat_data.tuner);

    timer_positioner_show_signal_reset();
	GUI_SetInterface("flush", NULL);
    switch(s_dish_moving_direct)
    {
        case DISH_MOVE_WEST:
			if(0 != strcasecmp(WND_POP_BOOK, GUI_GetFocusWindow()))
			{
				GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r");
			}
            break;

        case DISH_MOVE_EAST:
			if(0 != strcasecmp(WND_POP_BOOK, GUI_GetFocusWindow()))
			{
				GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l");
			}
            break;

        default:
            break;
    }

    s_dish_moving_direct = DISH_MOVE_NONE;

    return 0;
}

static char * app_positioner_get_focus_item_img_name(int sel,PositionerStatus status)
{
    char *image = NULL ;

    if (APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE ) {
        image =  "s_bar_half_focus_l" ;
    }
    else{
        image = "s_bar_half_focus_ok" ;
    }

#if (UI_MIRROR_SUPPORT > 0)
    extern  int app_menu_mirror_get(void);
    if(app_menu_mirror_get())
    {
        image = IMG_BAR_HALF_FOCUS_L;

        if(sel == 4)
        {
            sel = 3;
        }
        else if(sel == 6)
        {
            sel = 4;
        }

        if(sel < 5)
        {
            if(status == MOTOR_BUTTON_OK_SHOW)
            {
                GUI_SetProperty(s_positioner_ok_choice_tag[sel], "state", "show");
            }
            else
            {
                GUI_SetProperty(s_positioner_ok_choice_tag[sel], "state", "hide");
            }
        }
    }
#endif
    return (char*)image;
}

static void _positioner_focus_item(int sel)
{
#define MS_FOCUS_FORE_COLOR "[text_focus_color,text_color,text_disable_color]"
    char *img = IMG_BAR_HALF_FOCUS_L;

    if((sel == 0) || (sel == 1) || (sel == 2))
    {
        img = app_positioner_get_focus_item_img_name(sel,MOTOR_BUTTON_OK_SHOW);
    }
    if(s_positioner_mode == POSITIONER_DISEQC12)
    {
        if((sel == 4) || (sel == 6))
        {
            img = app_positioner_get_focus_item_img_name(sel,MOTOR_BUTTON_OK_SHOW);
        }
    }
    if(s_positioner_mode == POSITIONER_USALS)
    {
        uint32_t t_sel = 0;
        if(sel == 3)
            GUI_SetProperty("edit_positioner_longitude", "select", &t_sel);
        else if(sel == 4)
            GUI_SetProperty("edit_positioner_latitude", "select", &t_sel);
    }
    GUI_SetProperty(s_positioner_choice_back[sel], "img", img);
    GUI_SetProperty(s_positioner_item_title[sel], "string", s_positioner_title_content[s_positioner_mode][sel]);
    GUI_SetProperty(s_positioner_item_title[sel], "forecolor", MS_FOCUS_FORE_COLOR);//the color of text is unfocus color

    GUI_SetProperty(s_positioner_item_widget[s_positioner_mode][sel], "state", "focus");
}

static void _positioner_unfocus_item(int sel)
{
#define MS_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
    GUI_SetProperty(s_positioner_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
    GUI_SetProperty(s_positioner_item_title[sel], "string", s_positioner_title_content[s_positioner_mode][sel]);
    GUI_SetProperty(s_positioner_item_title[sel], "forecolor", MS_UNFOCUS_FORE_COLOR);//the color of text is unfocus color
    app_positioner_get_focus_item_img_name(sel,MOTOR_BUTTON_OK_HIDE);
}

static void _positioner_off_view(void)
{
	int i;

	GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][3], "state", "hide");
	GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][3], "state", "hide");

	for(i = 4; i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "state", "hide");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "hide");
        app_positioner_get_focus_item_img_name(i,MOTOR_BUTTON_OK_HIDE);
	}

	GUI_SetProperty(s_positioner_item_title[3], "string", s_positioner_title_content[POSITIONER_OFF][3]);
	GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][3], "state", "show");
	GUI_SetProperty(s_positioner_item_title[3], "state", "show");
	GUI_SetProperty(s_positioner_choice_back[3], "state", "show");
}

static void _positioner_diseqc12_view(void)
{
	int i;

	for(i = 3;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "string", s_positioner_title_content[POSITIONER_DISEQC12][i]);
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "show");
		GUI_SetProperty(s_positioner_item_title[i], "state", "show");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "show");
	}
}

static void _positioner_usals_view(void)
{
	int init_value = 0;
	char buff[10] = {0};
	int i;

	for(i = 3;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "string", s_positioner_title_content[POSITIONER_USALS][i]);
	}

	GxBus_ConfigGetInt(LONGITUDE_KEY, &init_value, LONGITUDE);
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", init_value/10, init_value%10);
	GUI_SetProperty("edit_positioner_longitude", "string", buff);

	GxBus_ConfigGetInt(LONGITUDE_DIRECT_KEY, &init_value, LONGITUDE_DIRECT);
	s_local_longitude_direct = init_value;
	memset(buff, 0, sizeof(buff));
	if(s_local_longitude_direct == LONGITUDE_EAST) //E
		buff[0] = 'E';
	else
		buff[0] = 'W';
	GUI_SetProperty("text_positioner_longitude_direct", "string", buff);

	GxBus_ConfigGetInt(LATITUDE_KEY, &init_value, LATITUDE);
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%02d.%d", init_value/10, init_value%10);
	GUI_SetProperty("edit_positioner_latitude", "string", buff);

	GxBus_ConfigGetInt(LATITUDE_DIRECT_KEY, &init_value, LATITUDE_DIRECT);
	s_local_latitude_direct= init_value;
	memset(buff, 0, sizeof(buff));
	if(s_local_latitude_direct == LATITUDE_SOUTH)
		buff[0] = 'S';
	else
		buff[0] = 'N';
	GUI_SetProperty("text_positioner_latitude_direct", "string", buff);

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude/10, s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude%10);
	buff[5]= ' ';
	if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST)
	{
		buff[6] = 'E';
	}
	else if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST)
	{
		buff[6] = 'W';
	}
	else{;}
	GUI_SetProperty("btn_positioner_longitude", "string", buff);

	for(i = 0;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "show");
		GUI_SetProperty(s_positioner_item_title[i], "state", "show");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "show");
	}
}

static void _diseqc12_set_pos_record(uint8_t pos)
{
	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		s_diseqc12_pos_map[pos -1] = 1;
	}
}

static void _diseqc12_clear_pos_record(uint8_t pos)
{
	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		s_diseqc12_pos_map[pos -1] = 0;
	}
}

static void _diseqc12_init_pos_record(void)
{
	int i;
	GxMsgProperty_NodeByPosGet node;

	memset(s_diseqc12_pos_map, 0, sizeof(s_diseqc12_pos_map));

	for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
	{
		node.node_type = NODE_SAT;
		node.pos = s_dvbs_sat_data.sat_buf[i];
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

		if((node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
		{
			_diseqc12_set_pos_record(node.sat_data.sat_s.diseqc12_pos);
		}
	}
}

static uint8_t _diseqc12_get_valid_pos(void)
{
	int i;

	for(i = 0; i < MAX_DISEQC12_POS; i++)
	{
		if(s_diseqc12_pos_map[i] == 0)
		{
			return i + 1;
		}
	}

	return 0;
}

static bool _diseqc12_check_pos_record(uint8_t pos)
{
	bool ret = false;

	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		if(s_diseqc12_pos_map[pos -1] == 1)
		{
			ret = true;
		}
	}

	return ret;
}

static status_t _diseqc12_save_position(void)
{
	GxMsgProperty_NodeModify node_modify;
	uint8_t pos =0;

	if(_diseqc12_check_pos_record(s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos) == false)
	{
		pos = _diseqc12_get_valid_pos();
		if(pos > 0)
		{
			_diseqc12_set_pos_record(pos);
			s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version |= GXBUS_PM_SAT_DISEQC_1_2;
			s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_USALS);
			s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos = pos;

			node_modify.node_type = NODE_SAT;
			memcpy(&node_modify.sat_data, &s_dvbs_sat_data.cur_sat.sat_data, sizeof(GxBusPmDataSat));
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
		}
		else
		{
			return GXCORE_ERROR;
		}
	}

	app_diseqc12_save_position(s_dvbs_sat_data.cur_sat.sat_data.tuner, s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos);

	return GXCORE_SUCCESS;
}


static status_t _usals_save_param(void)
{
    int longitude;
    int latitude;
    char *str = NULL;

    GxMsgProperty_NodeModify node_modify;
    status_t ret = GXCORE_SUCCESS;

	s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_1_2);
	s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version |= GXBUS_PM_SAT_DISEQC_USALS;
    _diseqc12_clear_pos_record(s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos);
    node_modify.node_type = NODE_SAT;
	memcpy(&node_modify.sat_data, &s_dvbs_sat_data.cur_sat.sat_data, sizeof(GxBusPmDataSat));
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

    GxBus_ConfigSetInt(LONGITUDE_DIRECT_KEY, s_local_longitude_direct);
    GxBus_ConfigSetInt(LATITUDE_DIRECT_KEY, s_local_latitude_direct);

    GUI_GetProperty("edit_positioner_longitude", "string", &str);
    longitude = 10 * app_float_edit_str_to_value(str);

    GUI_GetProperty("edit_positioner_latitude", "string", &str);
    latitude = 10 * app_float_edit_str_to_value(str);

    GxBus_ConfigSetInt(LONGITUDE_KEY, longitude);
    GxBus_ConfigSetInt(LATITUDE_KEY, latitude);

    return ret;
}

static status_t _positioner_off(void)
{
	GxMsgProperty_NodeModify node_modify;
	status_t ret = GXCORE_SUCCESS;

	_diseqc12_clear_pos_record(s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos);
	s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_1_2);
	s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_USALS);
	s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos = 0;
	node_modify.node_type = NODE_SAT;
	memcpy(&node_modify.sat_data, &s_dvbs_sat_data.cur_sat.sat_data, sizeof(GxBusPmDataSat));
	app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

	return ret;
}

static int32_t _positioner_motor_poplist_cb(int32_t ret_sel, unsigned short key)
{
    if(s_positioner_sel == 0) //sat list
    {
        GUI_SetProperty("cmb_positioner_sat_opt", "select", &ret_sel);
        poplist_destory_cb_free_item_content();
    }
    else if(s_positioner_sel == 1) // tp list
    {
        GUI_SetProperty("cmb_positioner_tp_opt", "select", &ret_sel);
        poplist_destory_cb_free_item_content();
    }
    else if(s_positioner_sel == 2)// motor type
    {
        GUI_SetProperty("cmb_positioner_motor_opt", "select", &ret_sel);
    }
    return 0;
}

static int _positioner_motor_pop(int type, int sel)
{
	PopList pop_list;
	int ret =-1;
	char *p_item = NULL ;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.sel = sel;

	if ( type == 0 )
	{
		ret = s_dvbs_sat_data.sat_poplist_get_name(&p_item);
		if (ret < 0)
			return -1 ;
		pop_list.title = STR_ID_SATELLITE;
		pop_list.item_num = s_dvbs_sat_data.sat_total;
		pop_list.item_content = (char**)p_item;
		pop_list.show_num= true;
	}
	else if (type == 1)
	{
		ret = s_dvbs_sat_data.tp_poplist_get_info(&p_item);
		if (ret < 0)
			return -1 ;
		pop_list.title = STR_ID_TP;
		pop_list.item_num = s_dvbs_sat_data.cur_sat_tp_total;
		pop_list.item_content = (char**)p_item;
		pop_list.show_num= true;
	}
	else if (type == 2)
	{
		pop_list.title = STR_ID_MOTOR_TYPE;
		pop_list.item_num = sizeof(s_moto_type_content)/sizeof(*s_moto_type_content);
		pop_list.item_content = s_moto_type_content;
		pop_list.show_num= false;
	}
	else
	{
		return -1 ;
	}
	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _positioner_motor_poplist_cb ;

	ret = poplist_create(&pop_list);

	return ret;
}

SIGNAL_HANDLER int app_positioner_setting_create(GuiWidget *widget, void *usrdata)
{
#if SAT2IP_SERVER_SUPPORT
    extern void app_sat2ip_set_dialog_pause(void);
    app_sat2ip_set_dialog_pause();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif

#if (0 == DOUBLE_S2_SUPPORT)
    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
#else
    s_dvbs_sat_data.tuner_rebuild(CMBOX_POSITIONER_TUNER);
#endif
	s_dvbs_sat_data.sat_rebuild("cmb_positioner_sat_opt", false);
	GUI_SetProperty("cmb_positioner_sat_opt","select",&s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);

    //get total sat num
	s_dvbs_sat_data.tp_rebuild("cmb_positioner_tp_opt");
	GUI_SetProperty("cmb_positioner_tp_opt","select",&s_dvbs_sat_data.cur_tp_sel);
	_diseqc12_init_pos_record();
	s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);

    if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
    {
        s_positioner_mode = POSITIONER_DISEQC12;
        _positioner_diseqc12_view();
        s_cur_max_item = MAX_POSITIONER_ON_ITEM;
    }
    else if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
    {
        s_positioner_mode = POSITIONER_USALS;
        _positioner_usals_view();
        s_cur_max_item = MAX_POSITIONER_ON_ITEM;
    }
    else
    {
        s_positioner_mode = POSITIONER_OFF;
        _positioner_off_view();
        s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
    }
#if DOUBLE_S2_SUPPORT
    if(TUNER_ALL == s_dvbs_sat_data.which_tuner_sat_exist)
        GUI_SetProperty(CMBOX_POSITIONER_TUNER, "select", &s_dvbs_sat_data.cur_tuner);
#endif
    GUI_SetProperty("cmb_positioner_motor_opt", "select", &s_positioner_mode);
    _positioner_unfocus_item(s_positioner_sel);
    s_positioner_sel = 0;
    _positioner_focus_item(s_positioner_sel);

	app_positioner_clean_signal();

	PositionVal sat_position;
	_positioner_sat_position_get(&sat_position);
	s_dvbs_sat_data.set_diseqc(&sat_position);
	s_dvbs_sat_data.set_tp(SET_TP_FORCE);

	APP_TIMER_ADD(sp_PosiTimerSignal, timer_positioner_show_signal, 100, TIMER_REPEAT);
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_setting_destroy(GuiWidget *widget, void *usrdata)
{
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;
    APP_TIMER_REMOVE(sp_PosiTimerSignal);
    APP_TIMER_REMOVE(sp_PosiLockTimer);
    app_epg_info_clean();
    app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_MONITOR_SET, &monitor);
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_cmb_tuner_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cur_tuner_bak = 0;
	PositionVal sat_position;

    if(s_dvbs_sat_data.which_tuner_sat_exist != TUNER_ALL)
        return EVENT_TRANSFER_STOP;

    cur_tuner_bak = s_dvbs_sat_data.cur_tuner;
    GUI_GetProperty(CMBOX_POSITIONER_TUNER, "select", &s_dvbs_sat_data.cur_tuner);

    if(cur_tuner_bak == s_dvbs_sat_data.cur_tuner)
        return EVENT_TRANSFER_STOP;

	s_dvbs_sat_data.sat_rebuild("cmb_positioner_sat_opt", false);
	GUI_SetProperty("cmb_positioner_sat_opt","select",&s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);

    //get total sat num
	s_dvbs_sat_data.tp_rebuild("cmb_positioner_tp_opt");
	GUI_SetProperty("cmb_positioner_tp_opt","select", &s_dvbs_sat_data.cur_tp_sel);
	_diseqc12_init_pos_record();
	s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);

    if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
    {
        s_positioner_mode = POSITIONER_DISEQC12;
        _positioner_diseqc12_view();
        s_cur_max_item = MAX_POSITIONER_ON_ITEM;
    }
    else if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
    {
        s_positioner_mode = POSITIONER_USALS;
        _positioner_usals_view();
        s_cur_max_item = MAX_POSITIONER_ON_ITEM;
    }
    else
    {
        s_positioner_mode = POSITIONER_OFF;
        _positioner_off_view();
        s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
    }
    GUI_SetProperty("cmb_positioner_motor_opt", "select", &s_positioner_mode);

	app_positioner_clean_signal();

	_positioner_sat_position_get(&sat_position);
	s_dvbs_sat_data.set_diseqc(&sat_position);
	s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    timer_positioner_show_signal_reset();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_cmb_tuner_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

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
                case STBK_UP:
                case STBK_DOWN:
                    s_positioner_sel = (event->key.sym == STBK_UP) ? s_cur_max_item - 1 : 0;
                    _positioner_focus_item(s_positioner_sel);
                    ret = EVENT_TRANSFER_STOP;
                    break;

                default:
                    break;
            }

        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_setting_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

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
                case VK_BOOK_TRIGGER:
                    {
                        //update gui sel in the mode of TUNER_ALL
                        #if DOUBLE_S2_SUPPORT
                        GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                        memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = TUNER_ALL;
                        s_dvbs_sat_data.sat_gui_sel_update();
                        //reset original tuner and sat data
                        memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = cur_sat_bak.sat_data.tuner;
                        #endif
                        GUI_EndDialog("after wnd_full_screen");
                        GUI_SetInterface("flush", NULL);
                    }
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    {
                        //update gui sel in the mode of TUNER_ALL
                        #if DOUBLE_S2_SUPPORT
                        GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                        memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = TUNER_ALL;
                        s_dvbs_sat_data.sat_gui_sel_update();
                        //reset original tuner and sat data
                        memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = cur_sat_bak.sat_data.tuner;
                        #endif
                        GUI_EndDialog(WND_POSITIONER_SETTING);
                    }
                    break;

                case STBK_UP:
                    _positioner_unfocus_item(s_positioner_sel);
                    if(0 == s_positioner_sel)
                    {
#if DOUBLE_S2_SUPPORT
                        if( APP_THEME_CLASSIC_BLUE ){
                            GUI_SetFocusWidget(CMBOX_POSITIONER_TUNER);
                        }else{
                            s_positioner_sel = s_cur_max_item - 1;
                            _positioner_focus_item(s_positioner_sel);
                        }
#else
                        s_positioner_sel = s_cur_max_item - 1;
                        _positioner_focus_item(s_positioner_sel);
#endif
                    }
                    else
                    {
                        s_positioner_sel--;
                        _positioner_focus_item(s_positioner_sel);
                    }
                    break;

                case STBK_DOWN:
                    _positioner_unfocus_item(s_positioner_sel);
                    if(s_positioner_sel == s_cur_max_item - 1)
                    {

#if DOUBLE_S2_SUPPORT
                        if( APP_THEME_CLASSIC_BLUE ){
                            GUI_SetFocusWidget(CMBOX_POSITIONER_TUNER);
                        }else{
                            s_positioner_sel = 0;
                            _positioner_focus_item(s_positioner_sel);
                        }
#else
                        s_positioner_sel = 0;
                        _positioner_focus_item(s_positioner_sel);
#endif
                    }
                    else
                    {
                        s_positioner_sel++;
                        _positioner_focus_item(s_positioner_sel);
                    }
                    break;

                case STBK_OK:
                    {
						int cmb_sel;
                        const char *focus_widget = NULL;

                        focus_widget = GUI_GetFocusWidget();
                        if(focus_widget != NULL
                                && strcmp(CMBOX_POSITIONER_TUNER, focus_widget))
                        {
                            if((s_positioner_sel == 0) && (s_dvbs_sat_data.sat_total > 0))//sat list
                            {
                                GUI_GetProperty("cmb_positioner_sat_opt", "select", &cmb_sel);
                                _positioner_motor_pop(s_positioner_sel,cmb_sel);
                            }
                            else if((s_positioner_sel == 1) && (s_dvbs_sat_data.cur_sat_tp_total > 0))// tp list
                            {
                                GUI_GetProperty("cmb_positioner_tp_opt", "select", &cmb_sel);
								_positioner_motor_pop(s_positioner_sel,cmb_sel);
                            }
                            else if(s_positioner_sel == 2)// motor type
                            {
                                cmb_sel = _positioner_motor_pop(s_positioner_sel,s_positioner_mode);
                            }
                        }
                        break;
                    }

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_setting_got_focus(GuiWidget *widget, void *usrdata)
{
	char *item_widget = NULL;
	char *left_img = IMG_BAR_HALF_FOCUS_L;
	char *right_img = IMG_BAR_HALF_UNFOCUS;
	char *property = "unfocus_img";
    if(strcmp(GUI_GetFocusWidget(),CMBOX_POSITIONER_TUNER)==0)
        return 0;
	/////////////left///////////////
	if((s_positioner_sel == 0) || (s_positioner_sel == 1) || (s_positioner_sel == 2))
	{
		left_img = app_positioner_get_focus_item_img_name(s_positioner_sel,MOTOR_BUTTON_OK_SHOW);
	}
	if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		if((s_positioner_sel == 4) || (s_positioner_sel == 6))
		{
			left_img = app_positioner_get_focus_item_img_name(s_positioner_sel,MOTOR_BUTTON_OK_SHOW);
		}
	}
	GUI_SetProperty(s_positioner_choice_back[s_positioner_sel], "img", left_img);
	GUI_SetProperty(s_positioner_item_title[s_positioner_sel], "string", s_positioner_title_content[s_positioner_mode][s_positioner_sel]);

	/////////////right///////////
	if(s_positioner_mode == POSITIONER_OFF)
	{
		item_widget = s_positioner_item_widget[POSITIONER_OFF][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		item_widget = s_positioner_item_widget[POSITIONER_DISEQC12][s_positioner_sel];
		if(s_positioner_sel == 3)
		{
			 GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l");
			 GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r");
		}
	}
	else if(s_positioner_mode == POSITIONER_USALS)
	{
		if(s_positioner_sel == 3) //longitude
		{
			item_widget = "boxitem_positioner_longitude";
			right_img = IMG_BAR_HALF_UNFOCUS_AR;
			property = "unfocus_image";
		}
		else if(s_positioner_sel == 4)//latitude
		{
			item_widget = "boxitem_positioner_latitude";
			right_img = IMG_BAR_HALF_UNFOCUS_AR;
			property = "unfocus_image";
		}
		else
		{
			item_widget =  s_positioner_item_widget[POSITIONER_USALS][s_positioner_sel];
		}
	}

	if(item_widget != NULL)
	{
		GUI_SetProperty(item_widget, property, right_img);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
	char *item_widget = NULL;
	char *img_l = IMG_BAR_HALF_FOCUS_L;
	char *img_r = NULL ;

	if (APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE ) {
		img_r =  "[T2_64_button_menu_m,T2_64_button_menu_m,T2_64_button_menu_r]" ;
	}
	else{
		img_r = "s_bar_half_focus_r" ;
	}

	char *property = "unfocus_img";
    if(strcmp(GUI_GetFocusWidget(),CMBOX_POSITIONER_TUNER)==0)
        return 0;
	//////////left img//////////////
    app_positioner_get_focus_item_img_name(s_positioner_sel,MOTOR_BUTTON_OK_HIDE);
	GUI_SetProperty(s_positioner_choice_back[s_positioner_sel], "img", img_l);
	GUI_SetProperty(s_positioner_item_title[s_positioner_sel], "string", s_positioner_title_content[s_positioner_mode][s_positioner_sel]);


	///////////right img//////////////
	if(s_positioner_mode == POSITIONER_OFF)
	{
		item_widget = s_positioner_item_widget[POSITIONER_OFF][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		item_widget = s_positioner_item_widget[POSITIONER_DISEQC12][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_USALS)
	{
		if(s_positioner_sel == 3) //longitude
		{
			item_widget = "boxitem_positioner_longitude";
            img_r = IMG_BAR_HALF_FOCUS_AR;
			property = "unfocus_image";
		}
		else if(s_positioner_sel == 4)//latitude
		{
			item_widget = "boxitem_positioner_latitude";
			img_r = IMG_BAR_HALF_FOCUS_AR;
			property = "unfocus_image";
		}
		else//save
		{
			item_widget =  s_positioner_item_widget[POSITIONER_USALS][s_positioner_sel];
		}
	}

	if(item_widget != NULL)
	{
		GUI_SetProperty(item_widget, property, img_r);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_sat_change(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeByPosGet cur_sat_bak = {0};
	memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));

	uint32_t sel;

	GUI_GetProperty("cmb_positioner_sat_opt", "select", &sel);
	s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
	s_dvbs_sat_data.cur_sat_get();

	if(cur_sat_bak.sat_data.id == s_dvbs_sat_data.cur_sat.sat_data.id)
	{
		return EVENT_TRANSFER_STOP;
	}

	s_dvbs_sat_data.cur_tp_sel = 0;
	s_dvbs_sat_data.tp_rebuild("cmb_positioner_tp_opt");

	if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
		s_positioner_mode = POSITIONER_DISEQC12;
		_positioner_diseqc12_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}
	else if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		s_positioner_mode = POSITIONER_USALS;
		_positioner_usals_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}
	else
	{
		s_positioner_mode = POSITIONER_OFF;
		_positioner_off_view();
		s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
	}
	GUI_SetProperty("cmb_positioner_motor_opt", "select", &s_positioner_mode);

	APP_TIMER_ADD(sp_PosiLockTimer, timer_positioner_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

#if DOUBLE_S2_SUPPORT
	if(s_dvbs_sat_data.cur_sat.sat_data.tuner == 0)
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1");
	else if(s_dvbs_sat_data.cur_sat.sat_data.tuner == 1)
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner2");
	else
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1 & Tuner2");
#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_tp_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel;

	GUI_GetProperty("cmb_positioner_tp_opt", "select", &sel);
	s_dvbs_sat_data.cur_tp_sel = sel;
	s_dvbs_sat_data.cur_tp_get();

	APP_TIMER_ADD(sp_PosiLockTimer, timer_positioner_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_motor_type_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	int cmb_sel;

	GUI_GetProperty("cmb_positioner_motor_opt", "select", &cmb_sel);
	s_positioner_mode = cmb_sel;

	if(s_positioner_mode ==POSITIONER_OFF)
	{
		_positioner_off_view();
		s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		_positioner_diseqc12_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;

	}
	else if(s_positioner_mode ==POSITIONER_USALS)
	{
		_positioner_usals_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_fine_drive_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_OFF;
    app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_MONITOR_SET, &monitor);

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
				case STBK_RIGHT:
                    if(s_dish_moving_direct != DISH_MOVE_WEST)
                    {
                        timer_positioner_show_signal_stop();
                        GxFrontend_StopMonitor(s_dvbs_sat_data.cur_sat.sat_data.tuner);

						app_diseqc12_find_drive_drictory(s_dvbs_sat_data.cur_sat.sat_data.tuner,LONGITUDE_WEST);//west
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r_yellow");
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l");
                        s_dish_moving_direct = DISH_MOVE_WEST;
                    }
					APP_TIMER_ADD(sp_PosiArrow, timer_positioner_stop_fine_drive, 130, TIMER_REPEAT);
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_LEFT:
                    if(s_dish_moving_direct != DISH_MOVE_EAST)
                    {
                        timer_positioner_show_signal_stop();
                        GxFrontend_StopMonitor(s_dvbs_sat_data.cur_sat.sat_data.tuner);

						app_diseqc12_find_drive_drictory(s_dvbs_sat_data.cur_sat.sat_data.tuner,LONGITUDE_EAST);//east
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l_yellow");
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r");
                        s_dish_moving_direct = DISH_MOVE_EAST;
                    }
					APP_TIMER_ADD(sp_PosiArrow, timer_positioner_stop_fine_drive, 130, TIMER_REPEAT);
                    ret = EVENT_TRANSFER_STOP;
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_limit_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

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
                case STBK_OK:
                    if(s_positioner_mode == POSITIONER_DISEQC12)
                    {
                        int sel;
                        PopDlg  pop;

                        GUI_GetProperty("cmb_positioner_limit", "select", &sel);
						app_diseqc12_save_limit(s_dvbs_sat_data.cur_sat.sat_data.tuner,sel);

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_SAVING;
                        pop.create_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 2;
                        popdlg_create(&pop);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_recalc_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

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
                case STBK_OK:
                    if(s_positioner_mode == POSITIONER_DISEQC12)
                    {
                        PopDlg pop;

                        app_diseqc12_recalculate(s_dvbs_sat_data.cur_sat.sat_data.tuner,s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos);

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_SAVING;
                        pop.create_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 1;
                        popdlg_create(&pop);

                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_diseqc12_goto_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

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
				case STBK_OK:
				{
					int sel = 0;
					if(s_positioner_mode == POSITIONER_DISEQC12)
					{
						bool force = true;
                        MotorState tip = DISH_NONE;
                        int32_t motor_state = app_get_motor_state_in_fullscreen();

						GUI_GetProperty("cmb_positioner_diseqc12_goto", "select", &sel);

						if(sel == 0)
						{
							tip = app_diseqc12_goto_position(s_dvbs_sat_data.cur_sat.sat_data.id, s_dvbs_sat_data.cur_sat.sat_data.tuner,
									s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos, force, motor_state);
						}
						else if(sel == 1)
						{
							tip = app_diseqc12_goto_position(s_dvbs_sat_data.cur_sat.sat_data.id, s_dvbs_sat_data.cur_sat.sat_data.tuner,
									0, force, motor_state);
						}

                        app_create_position_tip(tip, motor_state);
						ret = EVENT_TRANSFER_STOP;
					}
					app_diseqc_sat_id_bak_set(s_dvbs_sat_data.cur_sat.sat_data.id);
				}
				break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_usals_goto_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

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
				case STBK_OK:
					if(s_positioner_mode == POSITIONER_USALS) //
					{
						bool force = true;

						LongitudeInfo sat_longitude = {0};
						PositionVal local_position;
						char *str = NULL;
                        MotorState tip = DISH_NONE;
                        int32_t motor_state = app_get_motor_state_in_fullscreen();

						local_position.longitude.longitude_direct = s_local_longitude_direct;
						GUI_GetProperty("edit_positioner_longitude", "string", &str);
						local_position.longitude.longitude = 10 * app_float_edit_str_to_value(str);

						local_position.latitude.latitude_direct = s_local_latitude_direct;
						GUI_GetProperty("edit_positioner_latitude", "string", &str);
						local_position.latitude.latitude = 10 * app_float_edit_str_to_value(str);

						sat_longitude.longitude = s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude;
						sat_longitude.longitude_direct = s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude_direct;
						tip = app_usals_goto_position(s_dvbs_sat_data.cur_sat.sat_data.id,
								s_dvbs_sat_data.cur_sat.sat_data.tuner,&sat_longitude,&local_position, force, motor_state);
                        app_create_position_tip(tip, motor_state);
						ret = EVENT_TRANSFER_STOP;
					}
					else
						;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_off_save_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

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
				case STBK_OK:
					if(s_positioner_mode == POSITIONER_OFF)
					{
						PopDlg  pop;

						_positioner_off();

						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_NO_BTN;
                        pop.mode = POP_MODE_UNBLOCK;
						pop.str = STR_ID_SAVING;
						pop.create_cb = NULL;
						pop.exit_cb = NULL;
						pop.timeout_sec= 1;
						popdlg_create(&pop);
					}

					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}


SIGNAL_HANDLER int app_positioner_on_save_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

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
				case STBK_OK:
				{
					PopDlg  pop;

					if(s_positioner_mode == POSITIONER_USALS)
					{
						if(_usals_save_param() == GXCORE_SUCCESS)
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_NO_BTN;
                            pop.mode = POP_MODE_UNBLOCK;
							pop.str = STR_ID_SAVING;
							pop.create_cb = NULL;
							pop.exit_cb = NULL;
							pop.timeout_sec = 1;
							popdlg_create(&pop);
						}
						else
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_ERR_POSITION;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
					}
					else if(s_positioner_mode == POSITIONER_DISEQC12)
					{
						if(_diseqc12_save_position() == GXCORE_SUCCESS)
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_NO_BTN;
                            pop.mode = POP_MODE_UNBLOCK;
							pop.str = STR_ID_SAVING;
							pop.create_cb = NULL;
							pop.exit_cb = NULL;
							pop.timeout_sec= 1;
							popdlg_create(&pop);
						}
						else
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_POS_FULL;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_local_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

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
				case STBK_UP:
				case STBK_DOWN:
					GUI_SendEvent(WND_POSITIONER_SETTING, event);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	char buff[2] = {0};

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
					if(s_local_longitude_direct == LONGITUDE_EAST)
					{
						buff[0] = 'W';
						s_local_longitude_direct = LONGITUDE_WEST;
					}
					else
					{
						buff[0] = 'E';
						s_local_longitude_direct = LONGITUDE_EAST;
					}
					GUI_SetProperty("text_positioner_longitude_direct", "string", buff);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_change(GuiWidget *widget, void *usrdata)
{
	int longitude = 0;
	char *atoi_str = NULL;
	char buff[10] = {0};

	GUI_GetProperty("edit_positioner_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
	if(longitude > 1800)
	{
		longitude = 1800;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", longitude/10, longitude%10);
	GUI_SetProperty("edit_positioner_longitude", "string", buff);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

	GUI_SetProperty("edit_positioner_longitude", "select", &sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	char buff[2] = {0};

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
					if(s_local_latitude_direct == LATITUDE_SOUTH)
					{
						buff[0] = 'N';
						s_local_latitude_direct = LATITUDE_NORTH;
					}
					else
					{
						buff[0] = 'S';
						s_local_latitude_direct = LATITUDE_SOUTH;
					}
					GUI_SetProperty("text_positioner_latitude_direct", "string", buff);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_change(GuiWidget *widget, void *usrdata)
{
	int latitude = 0;
	char *atoi_str = NULL;
	char buff[10] = {0};

	GUI_GetProperty("edit_positioner_latitude", "string", &atoi_str);
	latitude = 10 * app_float_edit_str_to_value(atoi_str);

	if(latitude > 900)
	{
		latitude = 900;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%02d.%d", latitude/10, latitude%10);
	GUI_SetProperty("edit_positioner_latitude", "string", buff);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

    GUI_SetProperty("edit_positioner_latitude", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

void app_positioner_setting_menu_exec(void)
{
    GxMsgProperty_NodeNumGet sat_num = {0};
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

    if(sat_num.node_num > 0)
    {
#if SAT2IP_SERVER_SUPPORT
        extern int app_sat2ip_operate_tip_get_force(void);
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
        GUI_CreateDialog(WND_POSITIONER_SETTING);
    }
    else
    {
        app_mainmenu_tip_exec(MM_NO_SAT);
    }
}

#endif
