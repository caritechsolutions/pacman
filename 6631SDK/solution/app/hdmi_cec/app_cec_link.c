#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <memmgr.h>
#include "app_cec.h"
#include "app_cec_private.h"
#include "app_cec_common_str.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_operation.h"
#include "app_cec_message_api.h"
#include "full_screen.h"
#include "module/channel_edit.h"
#include "module/app_time.h"
#include "module/app_log.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_volume_value.h"
#include "module/app_pmt.h"
#include "module/app_ioctl.h"
#include "app_book.h"

#define	DISP_BUF_SIZE 256
#define SEC_PER_DAY (60*60*24)
#define WND_CEC_MSG_POPTIP_X (280)
#define WND_CEC_MSG_POPTIP_Y (132)

static unsigned short cec_link_disp_cnt = 1;
static uint16_t g_digital_timer_prog_id = -1;
static E_CEC_LOGIC_ADDR g_source_address = CEC_LA_BROADCAST;

extern int app_create_menu_wnd(void);
extern int app_get_mute_state(void);
extern uint32_t app_system_vol_get(void);
extern int top_stbk_halt(uint32_t scan_code);
extern int Gui_WriteKeyBuf(unsigned int key);
extern int pop_dialog_create(PopDlgEx* dlg);
extern int app_pvr_stop_cb(PopDlgRet ret);
extern bool app_check_av_running(void);
extern bool app_program_need_predemux(GxMsgProperty_NodeByPosGet *node);
extern status_t app_get_current_program(GxMsgProperty_NodeByPosGet *node);
extern play_movie_ctrol_state app_get_play_movie_ctrol_state(void);
extern play_music_ctrol_state app_get_play_music_ctrol_state(void);

char *app_cec_convert_menu_lang_str_to_iso639_lang_str(void)
{
    int lang_value = 0;
	char *iso639_lang_str = NULL;

    GxBus_ConfigGetInt(OSD_LANG_KEY, &lang_value, OSD_LANG);
    if(strcmp(g_LangName[lang_value], "English") == 0)
    {
        iso639_lang_str = "eng";
    }
    else if(strcmp(g_LangName[lang_value], "Chinese") == 0)
    {
        iso639_lang_str = "chi";
    }
    else if(strcmp(g_LangName[lang_value], "Vietnamese") == 0)
    {
        iso639_lang_str = "vie";
    }
    else if(strcmp(g_LangName[lang_value], "Arabic") == 0)
    {
        iso639_lang_str = "ara";
    }
    else if(strcmp(g_LangName[lang_value], "Russian") == 0)
    {
        iso639_lang_str = "rus";
    }
    else if(strcmp(g_LangName[lang_value], "French") == 0)
    {
        iso639_lang_str =  "fra";
    }
    else if(strcmp(g_LangName[lang_value], "Portuguese") == 0)
    {
        iso639_lang_str = "por";
    }
    else if(strcmp(g_LangName[lang_value], "Turkish") == 0)
    {
        iso639_lang_str = "tur";
    }
    else if(strcmp(g_LangName[lang_value], "Spain") == 0)
    {
        iso639_lang_str = "spa";
    }
    else if(strcmp(g_LangName[lang_value], "Italian") == 0)
    {
        iso639_lang_str = "ita";
    }
    else if(strcmp(g_LangName[lang_value], "Polski") == 0)
    {
        iso639_lang_str = "pol";
    }
    else if(strcmp(g_LangName[lang_value], "German") == 0)
    {
        iso639_lang_str = "ger";
    }
    else if(strcmp(g_LangName[lang_value], "Farsi") == 0)
    {
        iso639_lang_str = "fas";
    }
    else if(strcmp(g_LangName[lang_value], "Czech") == 0)
    {
        iso639_lang_str = "ces";
    }
	else if(strcmp(g_LangName[lang_value], "Thai") == 0)
    {
        iso639_lang_str = "tha";
    }
    else
    {
        app_log_error("Unknown lang\n");
        iso639_lang_str = "eng";
    }
    return iso639_lang_str;
}

static char *app_cec_convert_iso639_lang_str_to_menu_lang_str(unsigned char* lang_str)
{
	char *menu_lang_str = NULL;

	if (lang_str == NULL)
	{
		return NULL;
	}
	if(memcmp("eng", lang_str, 3) == 0)
	{
		menu_lang_str = "English";
	}
	else if(memcmp("fre", lang_str, 3) == 0 || memcmp("fra", lang_str, 3) == 0)
	{
		menu_lang_str = "French";
	}
	else if(memcmp("ger", lang_str, 3) == 0 || memcmp("deu", lang_str, 3) == 0)
	{
		menu_lang_str = "German";
	}
	else if(memcmp("ita", lang_str, 3) == 0)
	{
		menu_lang_str = "Italian";
	}
	else if(memcmp("spa", lang_str, 3) == 0)
	{
		menu_lang_str = "Spain";
	}
	else if(memcmp("por", lang_str, 3) == 0)
	{
		menu_lang_str = "Portuguese";
	}
	else if(memcmp("rus", lang_str, 3) == 0)
	{
		menu_lang_str = "Russian";
	}
	else if(memcmp("tur", lang_str, 3) == 0)
	{
		menu_lang_str = "Turkish";
	}
	else if(memcmp("pol", lang_str, 3) == 0)
	{
		menu_lang_str = "Polski";
	}
	else if(memcmp("ara", lang_str, 3) == 0)
	{
		menu_lang_str = "Arabic";
	}
	else if(memcmp("vie", lang_str, 3) == 0)
	{
		menu_lang_str = "Vietnamese";
	}
	else if(memcmp("ces", lang_str, 3) == 0)
	{
		menu_lang_str = "Czech";
	}
	else if(memcmp("fas", lang_str, 3) == 0)
	{
		menu_lang_str = "Farsi";
	}
	else if(memcmp("chi", lang_str, 3) == 0)
	{
		menu_lang_str = "Chinese";
	}
	else if(memcmp("tha", lang_str, 3) == 0)
	{
		menu_lang_str = "Thai";
	}
	else
	{
	    // 11.2.6 - 4, Pass Criteria: The DUT menu language setting is not modified.
	    app_log_error("Unknown lang\n");
		menu_lang_str = NULL;
	}
	return menu_lang_str;
}

static int app_cec_link_set_menu_language(unsigned char* lang_str)
{
    int i = 0;
	int lang_value = 0;
	int ret = GXCORE_ERROR;
    char *menu_lang_str = NULL;

	if (lang_str == NULL)
	{
		return ret;
	}
	GxBus_ConfigGetInt(OSD_LANG_KEY, &lang_value, OSD_LANG);
	menu_lang_str = app_cec_convert_iso639_lang_str_to_menu_lang_str(lang_str);
	if (menu_lang_str)
	{
		for (i = 0; i < LANG_MAX; i++)
		{
		    if((strcmp(g_LangName[i], menu_lang_str) == 0)&&(lang_value != i))
		    {
		        lang_value = i;
		        GxBus_ConfigSetInt(OSD_LANG_KEY,lang_value);
				GUI_SetInterface("osd_language",g_LangName[i]);
#if (UI_MIRROR_SUPPORT > 0)
				app_menu_mirror_set(g_LangName[i]);
#endif
				ret = GXCORE_SUCCESS;
				break;
		    }
		}
	}
	return ret;
}

static int app_cec_link_up_or_down_normal(unsigned short key)
{
    if (g_AppChOps.get_list_total() != 0)
    {
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppFullArb.tip = FULL_STATE_DUMMY;
        g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
        if(STBK_UP == key)
        {
            g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_NEXT, 0);
        }
        else if(STBK_DOWN == key)
        {
            g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_PREV, 0);
        }
    }
    return 0;
}

static int app_cec_color_key_convert(unsigned char cec_key)
{
    int stb_color_key = GUIK_NULL;
	char* focus_win = NULL;

	focus_win = (char*)GUI_GetFocusWindow();
	if(focus_win && (0 == strcasecmp(focus_win, WND_FULL_SCREEN)))
	{
	    switch (cec_key)
		{
			case CEC_KEY_RED:
				stb_color_key = STBK_FIND;
				break;
			case CEC_KEY_GREEN:
				stb_color_key = STBK_SLEEP;
				break;
			case CEC_KEY_YELLOW:
				stb_color_key = STBK_YELLOW;
				break;
			case CEC_KEY_BLUE:
				stb_color_key = STBK_AUDIO;
				break;
			default:
				break;
		}
	}
	else
	{
	    switch (cec_key)
		{
			case CEC_KEY_RED:
				stb_color_key = STBK_RED;
				break;
			case CEC_KEY_GREEN:
				stb_color_key = STBK_GREEN;
				break;
			case CEC_KEY_YELLOW:
				stb_color_key = STBK_YELLOW;
				break;
			case CEC_KEY_BLUE:
				stb_color_key = STBK_BLUE;
				break;
			default:
				break;
		}
	}
	return stb_color_key;
}

int app_get_cec_keymap_data(bool is_receive, unsigned short *stb_key_val, unsigned int *cec_key_val)
{
#define KEYMAP_STR  (uint8_t*)"ceckeymap"
#define KEYMAP_GROUP  (uint8_t*)"group"
#define KEYMAP_NAME  (uint8_t*)"name"
#define RECEIVE_NAME  (uint8_t*)"receive"
#define SEND_NAME  (uint8_t*)"send"

#define CEC_KEYMAP_FILE "/dvb/theme/cec_keymap.xml"

	GXxmlDocPtr doc;
    int ret = GXCORE_ERROR;

    if((stb_key_val == NULL)&&(cec_key_val == NULL))
    {
        return GXCORE_ERROR;
    }

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(CEC_KEYMAP_FILE))
    {
        return GXCORE_ERROR;
    }
    doc = GXxmlParseFile(CEC_KEYMAP_FILE);
    if (NULL == doc)
    {
        return GXCORE_ERROR;
    }

    GXxmlNodePtr root = doc->root;
    if (root == NULL)
    {
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
    {
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    GXxmlNodePtr group = root->childs;
    while (NULL != group)
    {
        GXxmlNodePtr item = NULL;
        uint8_t *value = NULL;
        uint8_t *iname = NULL;

		iname = GXxmlGetProp(group, KEYMAP_NAME);
		if (iname)
		{
	        if ((is_receive == true)&&(GXxmlStrcmp(iname, RECEIVE_NAME) != 0))
	        {
	            group = group->next;
	            continue;
	        }
			else if ((is_receive == false)&&(GXxmlStrcmp(iname, SEND_NAME) != 0))
			{
			    group = group->next;
	            continue;
	        }
		}

        if (GXxmlStrcmp(group->name, KEYMAP_GROUP) == 0)
        {
            item =  group->childs;
            while (item != NULL)
            {
                iname = GXxmlGetProp(item, KEYMAP_NAME);
                if(iname == NULL)
                {
                    item = item->next;
                    continue;
                }

				int ret_key = find_sys_code((const char *)iname);
				if (is_receive == true)
				{
				    value = GXxmlNodeGetContent(item);
                    if(value != NULL)
                    {
						if (*cec_key_val == app_hexstr2value(value))
						{
						    switch (*cec_key_val)
							{
								case CEC_KEY_BLUE:
									*stb_key_val = app_cec_color_key_convert(*cec_key_val);
									break;
								case CEC_KEY_RED:
									*stb_key_val = app_cec_color_key_convert(*cec_key_val);
									break;
								case CEC_KEY_GREEN:
									*stb_key_val = app_cec_color_key_convert(*cec_key_val);
									break;
								case CEC_KEY_YELLOW:
									*stb_key_val = app_cec_color_key_convert(*cec_key_val);
									break;
								default:
									*stb_key_val = ret_key;
									break;
							}
							app_log_debug("##### value: 0x%d ######\n", ret_key);
							GXxmlFree(value);
	                        value = NULL;
	                        GXxmlFree(iname);
	                        iname = NULL;
							ret = GXCORE_SUCCESS;
	                        break;
						}
						else
						{
	                        GXxmlFree(value);
	                        value = NULL;
	                        GXxmlFree(iname);
	                        iname = NULL;
						}
                    }
				}
				else if((is_receive == false)&&(*stb_key_val == ret_key))
				{
				    value = GXxmlNodeGetContent(item);
                    if(value != NULL)
                    {
						*cec_key_val = app_hexstr2value(value);
						app_log_debug("##### value: 0x%02x ######\n", *cec_key_val);
                        GXxmlFree(value);
                        value = NULL;
                        GXxmlFree(iname);
                        iname = NULL;
						ret = GXCORE_SUCCESS;
                        break;
                    }
				}
				else
                {
                    ;
                }

                GXxmlFree(iname);
                iname = NULL;

                item = item->next;
            }
        }
        group = group->next;
    }

    GXxmlFreeDoc(doc);
    return ret;
}

int app_cec_stb_send_key(GUI_Event* event)
{
    unsigned short stb_key = 0;
	unsigned int cec_key_val = 0xff;

    if (event == NULL)
    {
        return -1;
    }

    stb_key = find_virtualkey_ex(event->key.scancode,event->key.sym);
	if (GXCORE_SUCCESS == app_get_cec_keymap_data(false, &stb_key, &cec_key_val))
	{
	    app_cec_msg_user_control_pressed(CEC_LA_TV, (E_CEC_KEY_CODE)cec_key_val);
	}
	return 0;
}

E_CEC_MENU_STATE app_cec_link_get_current_menu_activate_status(void)
{
	if((GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
	 ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_LIST))
	 ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG)))
    {
        return CEC_MENU_STATE_ACTIVATE;
    }
	else
	{
		return CEC_MENU_STATE_DEACTIVATE;
	}
}

static int app_cec_link_win_exist_to_win_widget(const char* widget_win_name)
{
	GUI_Event keyEvent = {0};
	int rtn = 1;

    if (NULL != widget_win_name)
    {
        char* focus_Window = (char*)GUI_GetFocusWindow();
        if (NULL != focus_Window)
        {
            rtn = strcasecmp(widget_win_name, focus_Window);
            while(rtn != 0)
            {
                if (NULL != focus_Window)
                {
                    if ((0 == strcasecmp(WND_FULL_SCREEN ,widget_win_name))&&
                            ((0 == strcasecmp(WND_TKGS_UPGRADE_PROCESS ,focus_Window))
                             ||(0 == strcasecmp(WND_POP_TIP ,focus_Window))
                             ||(0 == app_channel_info_compare_dialog(focus_Window))
                             ||(0 == strcasecmp(WND_COMMON_SELECT ,focus_Window))))
                    {
                        popdlg_destroy();
						GUI_EndDialog(focus_Window);
                    }
                    else if ((0 == strcasecmp(WIN_PIC_VIEW ,focus_Window))
                            ||(0 == strcasecmp(WIN_MUSIC_VIEW ,focus_Window)))
                    {
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
	return 0;
}

static unsigned int app_cec_link_timer_get_tick(void)
{
    GxTime time_now = {0};
    unsigned int time_msec = 0;

    GxCore_GetTickTime(&time_now);
    time_msec = time_now.seconds * 1000 + time_now.microsecs / 1000;
    return (time_msec);
}
static int app_cec_link_set_system_audio_mode_by_remote_cec_device(bool bOnOff)
{
    bool	bRecvFeatureAbortMessage = false;
	unsigned int  get_tick_start = 0;
	unsigned int  get_tick_end = 0;

	app_log_debug("(bOnOff=%s)\n", bOnOff?"On":"Off");
	CEC_FEATURE_ABORT_INFO last_cec_feature_abort_info = {0};
	if(bOnOff)
	{
		if(app_get_mute_state())
		{
			g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
		}
		app_cec_clear_last_received_feature_abort_info();
		app_cec_msg_set_system_audio_mode(CEC_LA_TV, TRUE);
		get_tick_start = app_cec_link_timer_get_tick();
		while(1)
		{
			get_tick_end = app_cec_link_timer_get_tick();
			if(app_cec_get_last_received_feature_abort_info(&last_cec_feature_abort_info))
			{
				if(	last_cec_feature_abort_info.source_address == CEC_LA_TV &&
					last_cec_feature_abort_info.opcode == OPCODE_SET_SYSTEM_AUDIO_MODE 
				)
				{
					app_log_error("TV does NOT support SAC feature!\n");
					bRecvFeatureAbortMessage = true;
					break;
				}
				else
				{
					app_log_debug("-src=0x%X, dst=0x%X, op=0x%X, re=0x%X\n", last_cec_feature_abort_info.source_address, last_cec_feature_abort_info.dest_address, last_cec_feature_abort_info.opcode, last_cec_feature_abort_info.reason);
				}
			}
			if((get_tick_end - get_tick_start) > 1500)
			{
				app_log_error("timeout  %d %d !!\n",get_tick_end, get_tick_start);
				break;
			}
			GxCore_ThreadDelay(100);
		}
		if(bRecvFeatureAbortMessage == false)
		{
			app_cec_msg_set_system_audio_mode(CEC_LA_BROADCAST, true);
		}
	}
	else
	{
		if(!app_get_mute_state())
		{
			g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
		}
		app_cec_msg_set_system_audio_mode(CEC_LA_BROADCAST, false);
	}
	return 0;
}

static int app_cec_link_digital_service_id(P_DIGITAL_SERVICE_ID pDigitalServiceID, E_CEC_OPCODE opcode)
{
    unsigned short ts_id = 0;
	unsigned short service_id = 0;
	unsigned short channel_number = 0;
	unsigned short channel_position = 0;
	unsigned short original_network_id = 0;
	GxMsgProperty_NodeByPosGet node_prog = {0};

	if (pDigitalServiceID == NULL)
	{
		return GXCORE_ERROR;
	}

	if(!pDigitalServiceID->service_id_method)
	{
		if(pDigitalServiceID->service_id.channel.ch_numr_format == 0x01)
		{
			channel_number =	pDigitalServiceID->service_id.channel.min_ch_num;
		}
		else if(pDigitalServiceID->service_id.channel.ch_numr_format == 0x02)
		{
			channel_number =	pDigitalServiceID->service_id.channel.major_ch_num;
		}
		else
		{
			return GXCORE_ERROR;
		}
		channel_position = channel_number - 1;
		if(channel_position != g_AppPlayOps.normal_play.play_count)
		{
		    if (opcode == OPCODE_RECORD_ON)
		    {
		        g_AppPlayOps.set_rec_time(0);
				g_AppPlayOps.program_play(PLAY_MODE_WITH_REC|PLAY_MODE_POINT, channel_position);
				g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		    }
			else if (opcode == OPCODE_SELECT_DIGITAL_SERVICE)
			{
				g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, channel_position);
				g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		    }
			else if (opcode == OPCODE_SET_DIGITAL_TIMER || opcode == OPCODE_CLEAR_DIGITAL_TIMER)
		    {
		        node_prog.node_type = NODE_PROG;
	            node_prog.pos = channel_position;
	            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
				g_digital_timer_prog_id = node_prog.prog_data.id;
		    }
			else
			{
				return GXCORE_ERROR;
			}
		}
		else
		{
		    if (opcode == OPCODE_SET_DIGITAL_TIMER || opcode == OPCODE_CLEAR_DIGITAL_TIMER)
		    {
		        node_prog.node_type = NODE_PROG;
	            node_prog.pos = channel_position;
	            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
				g_digital_timer_prog_id = node_prog.prog_data.id;
		    }
			app_log_error("Same Channel, No Need to Tuned!\n");
			return 1;
		}
	}
	else
	{
		/* Change Digital Service by ID*/
		switch(pDigitalServiceID->broadcast_system)
		{
			case BCAST_ARIB_T:
			case BCAST_ARIB_BS:
			case BCAST_ARIB_CS:
			case BCAST_ARIB_GENERIC:
				ts_id = pDigitalServiceID->service_id.arib.transport_stream_id;
				service_id = pDigitalServiceID->service_id.arib.service_id;
				original_network_id = pDigitalServiceID->service_id.arib.original_network_id;
			    break;
			case BCAST_ATSC_C:
			case BCAST_ATSC_T:
			case BCAST_ATSC_S:
			case BCAST_ATSC_GENERIC:
				ts_id = pDigitalServiceID->service_id.atsc.transport_stream_id;
				service_id = pDigitalServiceID->service_id.atsc.program_number;
			    break;
			case BCAST_DVB_C:
			case BCAST_DVB_T:
			case BCAST_DVB_S:
			case BCAST_DVB_S2:
			case BCAST_DVB_GENERIC:
				ts_id = pDigitalServiceID->service_id.dvb.transport_stream_id;
				service_id = pDigitalServiceID->service_id.dvb.service_id;
				original_network_id = pDigitalServiceID->service_id.dvb.original_network_id;
			    break;
		}
		for(channel_position = 0; channel_position < g_AppPlayOps.normal_play.play_total; channel_position++)
		{
		    node_prog.node_type = NODE_PROG;
            node_prog.pos = channel_position;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
			if(node_prog.prog_data.service_id == service_id)
			{
				if(node_prog.prog_data.ts_id == ts_id)
				{
					if(node_prog.prog_data.original_id == original_network_id
					    || pDigitalServiceID->broadcast_system == BCAST_ATSC_GENERIC
					    || pDigitalServiceID->broadcast_system == BCAST_ATSC_C
					    || pDigitalServiceID->broadcast_system == BCAST_ATSC_S
					    || pDigitalServiceID->broadcast_system == BCAST_ATSC_T)
					{
						channel_number = channel_position+1;
						if(channel_position != g_AppPlayOps.normal_play.play_count)
						{
						    if (opcode == OPCODE_RECORD_ON)
						    {
						        g_AppPlayOps.set_rec_time(0);
								g_AppPlayOps.program_play(PLAY_MODE_WITH_REC|PLAY_MODE_POINT, channel_position);
								g_AppFullArb.state.pause = STATE_OFF;
				                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
						    }
							else if (opcode == OPCODE_SELECT_DIGITAL_SERVICE)
							{
								g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, channel_position);
								g_AppFullArb.state.pause = STATE_OFF;
				                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
						    }
							else if (opcode == OPCODE_SET_DIGITAL_TIMER || opcode == OPCODE_CLEAR_DIGITAL_TIMER)
						    {
								g_digital_timer_prog_id = node_prog.prog_data.id;
						    }
							else
							{
								return GXCORE_ERROR;
							}
						}
						else
						{
						    if (opcode == OPCODE_SET_DIGITAL_TIMER || opcode == OPCODE_CLEAR_DIGITAL_TIMER)
						    {
								g_digital_timer_prog_id = node_prog.prog_data.id;
						    }
							app_log_error("Same Channel, No Need to Tuned!\n");
							return 1;
						}
					}
					else
					{
						app_log_error("No Channel match!\n");
						return GXCORE_ERROR;
					}
				}
			}
		}
	}
	return GXCORE_SUCCESS;
}


static bool app_cec_link_is_playing(void)
{
    char* focus_window = NULL;
	focus_window = (char*)GUI_GetFocusWindow();
	if(focus_window)
	{
	    if ((0 == strcasecmp(WND_FULL_SCREEN, focus_window))
	       ||(0 == strcasecmp(WIN_PIC_VIEW, focus_window))
	       ||(0 == strcasecmp(WIN_MUSIC_VIEW, focus_window))
           ||(0 == strcasecmp(WIN_MOVIE_VIEW, focus_window))
           ||(0 == strcasecmp(WND_NETVIDEO_PLAY, focus_window))
           ||(0 == strcasecmp(WND_COMMON_SELECT, focus_window))
           ||(0 == app_channel_info_compare_dialog(focus_window)))
	    {
	        return true;
	    }
	    else
	    {
	        return false;
	    }
	}
	else
    {
        return false;
    }
}

static int app_cec_link_get_tuner_device_Info(P_CEC_TUNER_DEVICE_INFO device_infor)
{
	unsigned int channel_number = 0;
	unsigned int channel_position = 0;
	unsigned char service_id_method = 0;
	unsigned char channel_num_format = 0;
	unsigned char digital_broadcast_system = 0;
	P_CEC_TUNER_DEVICE_INFO pDevInfo = {0};
	GxMsgProperty_NodeByPosGet node_prog = {0};

	if (device_infor == NULL)
	{
	    return GXCORE_ERROR;
	}
#if (DEMOD_DVB_S > 0)||(DEMOD_DVB_COMBO > 0)||(DOUBLE_S2_SUPPORT > 0)
    digital_broadcast_system = BCAST_DVB_S2;
#elif (DEMOD_DVB_C > 0)
    digital_broadcast_system = BCAST_DVB_C;
#elif (DEMOD_DVB_T > 0)
    digital_broadcast_system = BCAST_DVB_T;
#else
    digital_broadcast_system = BCAST_DVB_GENERIC;
#endif
    channel_position	= g_AppPlayOps.normal_play.play_count;
	channel_number =	channel_position + 1;
	node_prog.node_type = NODE_PROG;
    node_prog.pos = channel_position;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
	service_id_method = app_cec_get_service_id_method();
	channel_num_format =	app_cec_get_channel_number_format();
	pDevInfo = (P_CEC_TUNER_DEVICE_INFO)device_infor;
	pDevInfo->recording_flag	= (g_AppPvrOps.state == PVR_DUMMY) ? 0 : 1;
	pDevInfo->tuner_display_info	 = app_cec_link_is_playing() ? 0 : 1;
	pDevInfo->digital_service_id.service_id_method = service_id_method;
	pDevInfo->digital_service_id.broadcast_system = digital_broadcast_system;
	if(!pDevInfo->digital_service_id.service_id_method)
	{
		if(channel_num_format == CHANNEL_NUM_1_PART)
		{
			pDevInfo->digital_service_id.service_id.channel.reserved = 0;
			pDevInfo->digital_service_id.service_id.channel.major_ch_num = 0;
			pDevInfo->digital_service_id.service_id.channel.min_ch_num = channel_number&0xFFFF;
			pDevInfo->digital_service_id.service_id.channel.ch_numr_format = CHANNEL_NUM_1_PART;
		}
		else if(channel_num_format == CHANNEL_NUM_2_PART)
		{
            pDevInfo->digital_service_id.service_id.channel.reserved = 0;
			pDevInfo->digital_service_id.service_id.channel.min_ch_num = 0;
			pDevInfo->digital_service_id.service_id.channel.major_ch_num = channel_number&0xFFFF;
			pDevInfo->digital_service_id.service_id.channel.ch_numr_format = CHANNEL_NUM_2_PART;
		}
	}
	else
	{
		switch(pDevInfo->digital_service_id.broadcast_system)
		{
			case BCAST_ARIB_T:
			case BCAST_ARIB_BS:
			case BCAST_ARIB_CS:
			case BCAST_ARIB_GENERIC:
				pDevInfo->digital_service_id.service_id.arib.service_id = node_prog.prog_data.service_id;
				pDevInfo->digital_service_id.service_id.arib.transport_stream_id = node_prog.prog_data.ts_id;
				pDevInfo->digital_service_id.service_id.arib.original_network_id = node_prog.prog_data.original_id;
			break;
			case BCAST_ATSC_C:
			case BCAST_ATSC_T:
			case BCAST_ATSC_S:
			case BCAST_ATSC_GENERIC:
				pDevInfo->digital_service_id.service_id.atsc.program_number = node_prog.prog_data.service_id;
				pDevInfo->digital_service_id.service_id.atsc.transport_stream_id = node_prog.prog_data.ts_id;
			break;
			case BCAST_DVB_C:
			case BCAST_DVB_T:
			case BCAST_DVB_S:
			case BCAST_DVB_S2:
			case BCAST_DVB_GENERIC:
				pDevInfo->digital_service_id.service_id.dvb.service_id	= node_prog.prog_data.service_id;
				pDevInfo->digital_service_id.service_id.dvb.transport_stream_id	 = node_prog.prog_data.ts_id;
				pDevInfo->digital_service_id.service_id.dvb.original_network_id	 = node_prog.prog_data.original_id;			
			break;
		}
	}
	return GXCORE_SUCCESS;
}

static int app_cec_record_pop(char* str)
{
    if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = str;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.timeout_sec = 3;
        popdlg_create(&pop);
    }
    return 0;
}

static int app_cec_link_record_on(P_CEC_OPCODE_HANDLER opcode_handler)
{
    int ret = -1;
    AppFrontend_LockState lock;
	GxMsgProperty_NodeByPosGet node = {0};
	E_CEC_RECORD_STATUS_INFO record_status = NO_RECORD_OTHER_REASON;

	if(g_AppPvrOps.state == PVR_DUMMY)
	{
	    usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
		if(usb_state == USB_ERROR)
		{
			record_status = NO_RECORD_MEDIA_PROTECTED;
			app_cec_msg_record_status(opcode_handler->source_address, record_status);
            app_cec_record_pop(STR_ID_INSERT_USB);
			return GXCORE_ERROR;
		}
		else if(usb_state == USB_NO_SPACE)
		{
			record_status = NO_RECORD_NOT_ENOUGH_SPACE_AVAILABLE;
			app_cec_msg_record_status(opcode_handler->source_address, record_status);
            app_cec_record_pop(STR_ID_NO_SPACE);
			return GXCORE_ERROR;
		}
		else if(usb_state == USB_READ_ONLY)
		{
			record_status = NO_RECORD_MEDIA_PROTECTED;
			app_cec_msg_record_status(opcode_handler->source_address, record_status);
            app_cec_record_pop(STR_ID_NOWRITE_PERMISSIONS);
			return GXCORE_ERROR;
		}
	    if ((opcode_handler->param_length == CEC_MSG_PARAM_LEN_EIGHT)&&(opcode_handler->param[0] == 2))
		{
		   ret = app_cec_link_digital_service_id((P_DIGITAL_SERVICE_ID)&opcode_handler->param[1], OPCODE_RECORD_ON);
		   if(GXCORE_SUCCESS == ret)
		   {
		       record_status = RECORD_DIGITAL_SERVICE;
		       app_cec_msg_record_status(opcode_handler->source_address, record_status);
			   return GXCORE_SUCCESS;
		   }
		   else if(GXCORE_ERROR == ret)
		   {
			   app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			   return GXCORE_ERROR;
		   }
		   else
		   {
		       record_status = RECORD_DIGITAL_SERVICE;
		   }
		}
		else if ((opcode_handler->param_length == CEC_MSG_PARAM_LEN_ONE)&&(opcode_handler->param[0] == 1))
		{
			record_status = RECORD_CURRENT_SELECT_SOURCE;
		}
		else
		{
			app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			return GXCORE_ERROR;
		}
		if((GXCORE_SUCCESS != app_get_current_program(&node))
#if (0 == REDIO_RECODE_SUPPORT)
			||(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
#endif
			||(app_program_need_predemux(&node)))
		{
			app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
            app_cec_record_pop(STR_ID_CANT_PVR);
			return GXCORE_ERROR;
		}
		if((app_check_av_running() == true)&&(0xff != g_AppPmt.ver))
		{
			if(g_AppFullArb.state.pause == STATE_ON)
			{
				g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
				g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
			}
			if(app_pvr_rec_start(0) == -1)
			{
				record_status = NO_RECORD_MEDIA_PROBLEM;
				app_cec_msg_record_status(opcode_handler->source_address, record_status);
				app_cec_record_pop(STR_ID_CANT_PVR);
				return GXCORE_ERROR;
			}
			else
			{
				GUI_CreateDialog(WND_PVR_BAR);
				GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
			}
		}
		else if (g_AppFullArb.state.pause == STATE_ON)
		{
			g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
			g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
			g_AppPlayOps.set_rec_time(0);
			app_play_current_prog(PLAY_MODE_WITH_REC);
		}
		else
		{
			app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);

			node.node_type = NODE_PROG;
			node.pos = g_AppPlayOps.normal_play.play_count;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
			if((g_AppPlayOps.normal_play.parent_key == PLAY_KEY_LOCK
					|| g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK
					|| node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
					&& node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
					&& lock == FRONTEND_LOCKED)
			{
				if(app_pvr_rec_start(0) == -1)
				{
					record_status = NO_RECORD_MEDIA_PROBLEM;
					app_cec_msg_record_status(opcode_handler->source_address, record_status);
                    app_cec_record_pop(STR_ID_CANT_PVR);
					return GXCORE_ERROR;
				}
				if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
				{
					GUI_EndDialog(WND_PASSWD);
				}
				GUI_CreateDialog(WND_PVR_BAR);
				GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
			}
			else
			{
				record_status = NO_RECORD_MEDIA_PROTECTED;
			}
		}
	}
	else
	{
		record_status = NO_RECORD_ALREADY_RECORDING;
	}
	app_cec_msg_record_status(opcode_handler->source_address, record_status);
	return GXCORE_SUCCESS;
}

static int app_cec_stop_pvr_to_menu(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_menu_wnd();
    }
	app_cec_msg_menu_status(g_source_address, app_cec_link_get_current_menu_activate_status());
    return 0;
}

static E_CEC_DECK_STATUS_INFO app_cec_link_get_deck_status(void)
{
    E_CEC_DECK_STATUS_INFO deck_status_info = DECK_INFO_NO_MEDIA;

	if(g_AppPvrOps.state != PVR_DUMMY)
	{
		deck_status_info = DECK_INFO_RECORD;
	}
	else if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
	{
		play_music_ctrol_state music_state =  app_get_play_music_ctrol_state();
		switch (music_state) {
			case PLAY_MUSIC_CTROL_PREVIOUS:
				deck_status_info = DECK_INFO_SKIP_REVERSE_REWIND;
				break;
			case PLAY_MUSIC_CTROL_BACKWARD:
				deck_status_info = DECK_INFO_FAST_REVERSE;
				break;
			case PLAY_MUSIC_CTROL_PLAY:
				deck_status_info = DECK_INFO_PLAY;
				break;
			case PLAY_MUSIC_CTROL_PAUSE:
				deck_status_info = DECK_INFO_STILL;
				break;
			case PLAY_MUSIC_CTROL_RESUME:
				deck_status_info = DECK_INFO_PLAY;
				break;
			case PLAY_MUSIC_CTROL_FORWARD:
				deck_status_info = DECK_INFO_FAST_FORWARD;
				break;
			case PLAY_MUSIC_CTROL_NEXT:
				deck_status_info = DECK_INFO_SKIP_FORWARD_WIND;
				break;
			case PLAY_MUSIC_CTROL_STOP:
				deck_status_info = DECK_INFO_STOP;
				break;
			case PLAY_MUSIC_CTROL_RANDOM:
			default:
				deck_status_info = DECK_INFO_OTHER_STATUS;
				break;
		}
	}
	else if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
	{
		play_movie_ctrol_state movie_state =  app_get_play_movie_ctrol_state();
		switch (movie_state) {
			case PLAY_MOVIE_CTROL_PREVIOUS:
				deck_status_info = DECK_INFO_SKIP_REVERSE_REWIND;
				break;
			case PLAY_MOVIE_CTROL_BACKWARD:
				deck_status_info = DECK_INFO_FAST_REVERSE;
				break;
			case PLAY_MOVIE_CTROL_PLAY:
				deck_status_info = DECK_INFO_PLAY;
				break;
			case PLAY_MOVIE_CTROL_PAUSE:
				deck_status_info = DECK_INFO_STILL;
				break;
			case PLAY_MOVIE_CTROL_RESUME:
				deck_status_info = DECK_INFO_PLAY;
				break;
			case PLAY_MOVIE_CTROL_FORWARD:
				deck_status_info = DECK_INFO_FAST_FORWARD;
				break;
			case PLAY_MOVIE_CTROL_NEXT:
				deck_status_info = DECK_INFO_SKIP_FORWARD_WIND;
				break;
			case PLAY_MOVIE_CTROL_STOP:
				deck_status_info = DECK_INFO_STOP;
				break;
			case PLAY_MOVIE_CTROL_ZOOM:
			case PLAY_MOVIE_CTROL_RANDOM:
			default:
				deck_status_info = DECK_INFO_OTHER_STATUS;
				break;
		}
	}
	else
	{
		deck_status_info = DECK_INFO_NO_MEDIA;
	}
	return deck_status_info;
}

static int app_cec_link_set_deck_play_mode(E_CEC_DECK_PLAY_MODE deck_play_mode)
{
    int ret = GXCORE_SUCCESS;

	if ((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
	  ||(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW)))
	{
		switch (deck_play_mode) {
			case PLAY_MODE_PLAY_FORWARD:
				Gui_WriteKeyBuf(APPK_PLAY);
				break;
			case PLAY_MODE_PLAY_STILL:
				Gui_WriteKeyBuf(APPK_PAUSE);
				break;
			case PLAY_MODE_FAST_FORWARD_MIN_SPEED:
			case PLAY_MODE_FAST_FORWARD_MEDIUM_SPEED:
			case PLAY_MODE_FAST_FORWARD_MAX_SPEED:
			case PLAY_MODE_SLOW_FORWARD_MIN_SPEED:
			case PLAY_MODE_SLOW_FORWARD_MEDIUM_SPEED:
			case PLAY_MODE_SLOW_FORWARD_MAX_SPEED:
				Gui_WriteKeyBuf(APPK_FF);
				break;
			case PLAY_MODE_PLAY_REVERSE:
			case PLAY_MODE_FAST_REVERSE_MIN_SPEED:
			case PLAY_MODE_FAST_REVERSE_MEDIUM_SPEED:
			case PLAY_MODE_FAST_REVERSE_MAX_SPEED:
			case PLAY_MODE_SLOW_REVERSE_MIN_SPEED:
			case PLAY_MODE_SLOW_REVERSE_MEDIUM_SPEED:
			case PLAY_MODE_SLOW_REVERSE_MAX_SPEED:
				Gui_WriteKeyBuf(APPK_REW);
				break;
			default:
				ret = GXCORE_ERROR;
				break;
		}
	}
	else
	{
	    ret = GXCORE_ERROR;
	}
	return ret;
}

static int app_cec_link_set_deck_control_mode(E_CEC_DECK_CONTROL_MODE deck_control_mode)
{
    int ret = GXCORE_SUCCESS;

	if ((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
	  ||(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW)))
	{
		switch (deck_control_mode) {
			case DECK_CONTROL_MODE_STOP:
				Gui_WriteKeyBuf(APPK_STOP);
				break;
			case DECK_CONTROL_MODE_SKIP_FORWARD_WIND:
				Gui_WriteKeyBuf(APPK_NEXT);
				break;
			case DECK_CONTROL_MODE_SKIP_REVERSE_REWIND:
				Gui_WriteKeyBuf(APPK_PREVIOUS);
				break;
			case DECK_CONTROL_MODE_EJECT:
				Gui_WriteKeyBuf(APPK_BACK);
				break;
			default:
				ret = GXCORE_ERROR;
				break;
		}
	}
	else
	{
	    ret = GXCORE_ERROR;
	}
	return ret;
}

static int app_cec_link_set_digital_timer(P_CEC_OPCODE_HANDLER opcode_handler) //(P_CEC_DIGITAL_TIMER_INFO digital_timer)
{
    int ret = -1;
	GxBook book = {0};
	time_t time = 0;
	time_t temp_sec = 0;
	time_t duration = 0;
	time_t start_time = 0;
	struct tm temp_tm = {0};
	BookProgStruct prog_data = {0};
	WeekDay gmt_weekday = WEEK_SUN;
	WeekDay local_weekday = WEEK_SUN;
	E_CEC_TIMER_RECORDING_SEQUENCE timer_recording_sequence = CEC_TIMER_ONCE_ONLY;

	time = g_AppTime.utc_get(&g_AppTime);
	temp_sec = get_display_time_by_timezone(time);
	if(temp_sec >= 0)
	{
		time = temp_sec;
	}
	time = time - time % 60;
	localtime_r(&time, &temp_tm);
	temp_tm.tm_mday = opcode_handler->param[0];
	temp_tm.tm_mon = opcode_handler->param[1];
	temp_tm.tm_hour = opcode_handler->param[2];
	temp_tm.tm_min = opcode_handler->param[3];
	time = mktime(&temp_tm);
   	book.book_type = BOOK_PROGRAM_PVR;
	book.trigger_mode = BOOK_TRIG_BY_SEGMENT;
	duration = (opcode_handler->param[4]*60*60 + opcode_handler->param[5]*60);;
	timer_recording_sequence = (E_CEC_TIMER_RECORDING_SEQUENCE)(opcode_handler->param[6] & 0x7F);
	if(timer_recording_sequence == CEC_TIMER_ONCE_ONLY)
	{
		book.repeat_mode.mode = BOOK_REPEAT_ONCE;
	}
	else
	{
		if(timer_recording_sequence == CEC_TIMER_MONDAY)
		{
			local_weekday = WEEK_MON;
		}
		else if(timer_recording_sequence == CEC_TIMER_TUESDAY)
		{
			local_weekday = WEEK_TUES;
		}
		else if(timer_recording_sequence == CEC_TIMER_WEDNESDAY)
		{
			local_weekday = WEEK_WED;
		}
		else if(timer_recording_sequence == CEC_TIMER_THURSDAY)
		{
			local_weekday = WEEK_THURS;
		}
		else if(timer_recording_sequence == CEC_TIMER_FRIDAY)
		{
			local_weekday = WEEK_FRI;
		}
		else if(timer_recording_sequence == CEC_TIMER_SATURDAY)
		{
			local_weekday = WEEK_SAT;
		}
		else
		{
			local_weekday = WEEK_SUN;
		}
		gmt_weekday = app_weekday_local2gmt(local_weekday, time % SEC_PER_DAY);
		book.repeat_mode.mode = g_AppBook.wday2mode(gmt_weekday);
	}
	start_time = get_local_time_by_timezone(time);
	if(start_time < 0)
	{
        book.trigger_time_start = start_time + SEC_PER_DAY;
	}
	else
	{
        book.trigger_time_start = start_time;
	}
    book.trigger_time_advance = ADVANCE_BOOK_TIME;
	if(duration <= 0)
	{
		return GXCORE_ERROR;
	}
    book.trigger_time_end = book.trigger_time_start + 3;
	if (GXCORE_ERROR == app_cec_link_digital_service_id((P_DIGITAL_SERVICE_ID)&opcode_handler->param[7], OPCODE_SET_DIGITAL_TIMER))
	{
		return GXCORE_ERROR;
	}
	if(g_digital_timer_prog_id == -1)
	{
		return GXCORE_ERROR;
	}
	prog_data.prog_id = g_digital_timer_prog_id;
	prog_data.duration = duration;
    app_log_error("prog_data.duration = %d \n", prog_data.duration);
	book.struct_size = sizeof(BookProgStruct);
	memcpy(book.struct_buf, (uint8_t*)(&(prog_data)), book.struct_size);
	if (opcode_handler->opcode == OPCODE_SET_DIGITAL_TIMER)
	{
	    ret = g_AppBook.creat(&book);
	}
	else if (opcode_handler->opcode == OPCODE_CLEAR_DIGITAL_TIMER)
	{
	     ret = g_AppBook.remove(&book);
	}
	else
	{
	   ret = GXCORE_ERROR;
	}
	if (GXCORE_ERROR == ret)
	{
	    app_log_error("Invalid Timer! \n");
	}
    return ret;
}

static CEC_OPCODE_HANDLER g_opcode_handler = {0};
static bool g_opcode_handler_flag = false;

int app_cec_change_service(void)
{

    CEC_TUNER_DEVICE_INFO stTunerDeviceInfo = {0};
	if (g_opcode_handler_flag == true)
	{
	    g_opcode_handler_flag = false;
	    app_cec_link_get_tuner_device_Info(&stTunerDeviceInfo);
	    app_cec_msg_tuner_device_status(g_opcode_handler.source_address, &stTunerDeviceInfo);
	}
	return 0;
}

int app_cec_link_opcode_handler(P_CEC_OPCODE_HANDLER opcode_handler)
{
	unsigned int mute_state = 0;
	unsigned int audio_vol = 0;
	CEC_TIMER_STATUS_DATA timer_status = {0};
	CEC_TUNER_DEVICE_INFO stTunerDeviceInfo = {0};
	E_CEC_DECK_STATUS_INFO deck_status = DECK_INFO_NO_MEDIA;

	if (opcode_handler == NULL)
	{
	    return GXCORE_ERROR;
	}

	switch(opcode_handler->opcode)
	{
		case OPCODE_SYSTEM_STANDBY:
			if(g_AppPvrOps.state != PVR_DUMMY)
			{
				app_cec_msg_feature_abort(opcode_handler->source_address, OPCODE_SYSTEM_STANDBY, REASON_NOT_IN_CORRECT_MODE);
			}
			else
			{
				cec_feature_local_standby_mode();
				top_stbk_halt(OPCODE_SYSTEM_STANDBY);
			}
			break;
		case OPCODE_SET_MENU_LANGUAGE:
			app_cec_link_set_menu_language(opcode_handler->param);
			break;
		case OPCODE_GIVE_TUNER_DEVICE_STATUS:
			app_cec_link_get_tuner_device_Info(&stTunerDeviceInfo);
			if(opcode_handler->param[0] == CEC_STATUS_REQUEST_ON)
			{
			    memset(&g_opcode_handler,0x0,sizeof(g_opcode_handler));
				g_opcode_handler.opcode = opcode_handler->opcode;
				g_opcode_handler.param_length = opcode_handler->param_length;
				g_opcode_handler.dest_address = opcode_handler->dest_address;
				g_opcode_handler.source_address = opcode_handler->source_address;
				memcpy(g_opcode_handler.param, opcode_handler->param, opcode_handler->param_length);
				app_cec_add_tuner_status_subscriber(opcode_handler->source_address);
				app_cec_msg_tuner_device_status(opcode_handler->source_address, &stTunerDeviceInfo);
				g_opcode_handler_flag = true;
			}
			else if(opcode_handler->param[0] == CEC_STATUS_REQUEST_OFF)
			{
			    g_opcode_handler_flag = false;
				app_cec_remove_tuner_status_subscriber(opcode_handler->source_address);
			}
			else
			{
				app_cec_msg_tuner_device_status(opcode_handler->source_address, &stTunerDeviceInfo);
			}
			break;
		case OPCODE_SELECT_DIGITAL_SERVICE:
			if(GXCORE_ERROR == app_cec_link_digital_service_id((P_DIGITAL_SERVICE_ID)opcode_handler->param, opcode_handler->opcode))
			{
				app_log_error("No Need To Tune or Channel Info is Not Correct!\n");
			}
			break;
		case OPCODE_TUNER_STEP_DECREMENT:
			app_cec_link_up_or_down_normal(STBK_DOWN);
			break;
		case OPCODE_TUNER_STEP_INCREMENT:
			app_cec_link_up_or_down_normal(STBK_UP);
			break;
		case OPCODE_MENU_REQUEST:
			if(app_cec_get_source_active_state() == CEC_SOURCE_STATE_ACTIVE)
			{
				switch(opcode_handler->param[0])
				{
					case CEC_MENU_REQUEST_ACTIVATE:
						if(app_cec_link_get_current_menu_activate_status() == CEC_MENU_STATE_DEACTIVATE)
						{
							if(g_AppPvrOps.state != PVR_DUMMY)
                            {
								PopDlg pop;
								memset(&pop, 0, sizeof(PopDlg));
								pop.type = POP_TYPE_YES_NO;
								pop.format = POP_FORMAT_DLG;
							    pop.mode = POP_MODE_UNBLOCK;
							    pop.exit_cb = app_cec_stop_pvr_to_menu;
							    pop.str = app_pvr_get_tips(g_AppPvrOps.state);
							    popdlg_create(&pop);
								g_source_address = opcode_handler->source_address;
								break;
                            }
                            else
                            {
                                app_create_menu_wnd();
                            }
						}
						app_cec_msg_menu_status(opcode_handler->source_address, app_cec_link_get_current_menu_activate_status());
						break;
					case CEC_MENU_REQUEST_DEACTIVATE:
						if(app_cec_link_get_current_menu_activate_status() == CEC_MENU_STATE_ACTIVATE)
						{
							app_cec_link_win_exist_to_win_widget(WND_FULL_SCREEN);
						}
						app_cec_msg_menu_status(opcode_handler->source_address, app_cec_link_get_current_menu_activate_status());					
						break;
					case CEC_MENU_REQUEST_QUERY:
						app_cec_msg_menu_status(opcode_handler->source_address, app_cec_link_get_current_menu_activate_status());
						break;
					default:
						break;
				}

			}
			else
			{
				app_log_error("STB is NOT active source!!\n");
			}
			break;
		case OPCODE_USER_CTRL_PRESSED:
			if (!app_cec_get_use_tv_remote_control_enable_flag())
			{
			    break;
			}
			switch(opcode_handler->param[0])
			{
				case CEC_KEY_MUTE:
					if(app_cec_get_system_audio_control_feature())
					{
						g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
					}
					break;
				default:
					if(app_cec_get_remote_control_passthrough_feature())
					{
					    unsigned short keypress = GUIK_NULL;
						if (GXCORE_SUCCESS == app_get_cec_keymap_data(true, &keypress,(unsigned int *)&opcode_handler->param[0]))
						{
					        Gui_WriteKeyBuf(keypress);
						}
					}
					break;
			}
			app_cec_msg_user_control_pressed(opcode_handler->source_address,app_cec_get_remote_passthrough_key());
			break;
		case OPCODE_USER_CTRL_RELEASED:
			if (!app_cec_get_use_tv_remote_control_enable_flag())
			{
			    break;
			}
			app_cec_msg_user_control_released(opcode_handler->source_address);
			break;
		case OPCODE_GIVE_AUDIO_STATUS:
			app_cec_msg_report_audio_status(opcode_handler->source_address, ((app_get_mute_state()<<7)|(app_system_vol_get())));
			break;
		case OPCODE_REPORT_AUDIO_STATUS:
			mute_state = ((opcode_handler->param[0] & 0x80)>>7);
            audio_vol = (opcode_handler->param[0] & 0x7F);
			app_system_vol_set(audio_vol);
			if (((mute_state == 1)&& !app_get_mute_state())
			  ||((mute_state == 0)&& app_get_mute_state()))
			{
				g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
			}
			break;
		case OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
			if(app_cec_get_system_audio_control_feature())
			{
				app_cec_msg_system_audio_mode_status(opcode_handler->source_address);
			}
			else
			{
				app_log_error("SAC fucntion is NOT enabled!\n");
			}
			break;
		case OPCODE_SET_SYSTEM_AUDIO_MODE:
			if(opcode_handler->param[0] == 1)
			{
				app_cec_set_system_audio_mode_status(true);
				if(app_cec_get_system_audio_control_feature()&&(!app_get_mute_state()))
				{
					g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
				}
				else
				{
					app_log_error("SAC fucntion is NOT enabled!\n");
				}
			}
			else
			{
				app_cec_set_system_audio_mode_status(false);
				if(app_cec_get_system_audio_control_feature()&&(app_get_mute_state()))
				{
					g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
				}
				else
				{
					app_log_error("SAC fucntion is NOT enabled!\n");
				}
			}
			break;
		case OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
			if(app_cec_get_system_audio_control_feature())
			{
				if(opcode_handler->param_length == 2)
				{
					app_cec_link_set_system_audio_mode_by_remote_cec_device(true);
				}
				else
				{
					app_cec_link_set_system_audio_mode_by_remote_cec_device(false);
				}
			}
			else
			{
				app_log_error("SAC fucntion is NOT enabled!\n");
			}
			break;
		case OPCODE_RECORD_OFF:
			if(g_AppPvrOps.state != PVR_DUMMY)
			{
				app_pvr_stop_cb(POP_VAL_OK);
				app_cec_msg_record_status(opcode_handler->source_address, RECORD_TERMINATED_NORMALLY);
			}
			break;
        case OPCODE_RECORD_ON:
			app_cec_link_record_on(opcode_handler);
			break;
		case OPCODE_DECK_CONTROL:
			if (GXCORE_ERROR == app_cec_link_set_deck_control_mode((E_CEC_DECK_CONTROL_MODE)opcode_handler->param[0]))
			{
			    app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			}
            break;
        case OPCODE_GIVE_DECK_STATUS:
			deck_status = app_cec_link_get_deck_status();
			if(opcode_handler->param[0] == CEC_STATUS_REQUEST_ON)
			{
			    app_cec_msg_deck_status(opcode_handler->source_address, deck_status);
			}
			else if(opcode_handler->param[0] == CEC_STATUS_REQUEST_OFF)
			{
				app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			}
			else
			{
			    app_cec_msg_deck_status(opcode_handler->source_address, deck_status);
			}
            break;
        case OPCODE_PLAY:
			if (GXCORE_ERROR == app_cec_link_set_deck_play_mode((E_CEC_DECK_PLAY_MODE)opcode_handler->param[0]))
			{
			    app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			}
			break;
		case OPCODE_SET_DIGITAL_TIMER:
			if (GXCORE_ERROR == app_cec_link_set_digital_timer(opcode_handler))
			{
			    app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			}
			else
			{
				memset(&timer_status, 0x0, sizeof(timer_status));
				timer_status.timer_overlap_warning = 0;
				timer_status.media_info = 0x0;
				timer_status.programmed_indicator = 0;
			    app_cec_msg_timer_status(opcode_handler->source_address, &timer_status);
			}
			break;
		case OPCODE_CLEAR_DIGITAL_TIMER:
			if (GXCORE_ERROR == app_cec_link_set_digital_timer(opcode_handler))
			{
			    app_cec_msg_feature_abort(opcode_handler->source_address, opcode_handler->opcode, REASON_REFUSED);
			}
			else
			{
			    app_cec_msg_timer_clear_status(opcode_handler->source_address, TIMER_CLEARED);
			}
			break;
		default:
			app_log_error("Unknown CEC opcode handler(0x%02X) is requested,  need to update to support this command!\n", opcode_handler->opcode);
			break;

	}
    return GXCORE_SUCCESS;
}

int app_cec_link_show_osd_message(P_CEC_OPCODE_HANDLER opcode_handler)
{
    PopDlgEx cec_msg_dialog = {0};
	int i = 0;
	static char cec_disp_buffer[DISP_BUF_SIZE] = {0};
	char cec_temp_buffer[DISP_BUF_SIZE] = {0};

	if (opcode_handler == NULL)
	{
	    return GXCORE_ERROR;
	}

	if(g_AppPvrOps.state != PVR_DUMMY)
	{
		return GXCORE_ERROR;
	}

	if(app_cec_get_func_enable_flag())
	{
	    app_clean_fullscreen_tip(true, false, true);
		memset(cec_disp_buffer, 0, DISP_BUF_SIZE);
		sprintf(cec_disp_buffer,"[#%d] RecvFrom(0x%x):\n", cec_link_disp_cnt++, opcode_handler->source_address);
		memset(cec_temp_buffer, 0, sizeof(cec_temp_buffer));
		sprintf(cec_temp_buffer," -Direction: %s  ->  %s\n", cec_logical_addr_str[opcode_handler->source_address], cec_logical_addr_str[opcode_handler->dest_address]);
		strcat(cec_disp_buffer, cec_temp_buffer);
		memset(cec_temp_buffer, 0, sizeof(cec_temp_buffer));
		sprintf(cec_temp_buffer," -OpCode: %s\n", cec_opcode_str[opcode_handler->opcode]);
		strcat(cec_disp_buffer, cec_temp_buffer);
		memset(cec_temp_buffer, 0, sizeof(cec_temp_buffer));
		sprintf(cec_temp_buffer," -Param[%d]:", opcode_handler->param_length);
		strcat(cec_disp_buffer, cec_temp_buffer);
		memset(cec_temp_buffer, 0, sizeof(cec_temp_buffer));
		for(i = 0; i < opcode_handler->param_length; i++)
		{
			sprintf(cec_temp_buffer," %02x", opcode_handler->param[i]);
			strcat(cec_disp_buffer, cec_temp_buffer);
		}
		memset(cec_temp_buffer, 0, sizeof(cec_temp_buffer));
		sprintf(cec_temp_buffer,"\n");
		strcat(cec_disp_buffer, cec_temp_buffer);
		memset(&cec_msg_dialog, 0, sizeof(PopDlg));
		cec_msg_dialog.timeout_sec = 3;
		cec_msg_dialog.show_time = false;
		cec_msg_dialog.title = "CEC Message";
		cec_msg_dialog.str = cec_disp_buffer;
	    cec_msg_dialog.type = POP_TYPE_NO_BTN;
		cec_msg_dialog.format = POP_FORMAT_POPUP;
		cec_msg_dialog.pos.x = WND_CEC_MSG_POPTIP_X;
		cec_msg_dialog.pos.y = WND_CEC_MSG_POPTIP_Y;
		pop_dialog_create(&cec_msg_dialog);
	}
	return GXCORE_SUCCESS;
}

int app_send_cec_link_opcode_handler_msg(P_CEC_OPCODE_HANDLER opcode_handler)
{
    GxMessage *new_msg = NULL;
	AppMsg_CecUIControl *msg_param = NULL;

    if (opcode_handler == NULL)
	{
	    return GXCORE_ERROR;
	}

	new_msg = GxBus_MessageNew(APPMSG_HDMI_CEC_UI_CONTROL);
    msg_param = (AppMsg_CecUIControl*)GxBus_GetMsgPropertyPtr(new_msg, AppMsg_CecUIControl);
    if(msg_param != NULL)
    {
        msg_param->type = CEC_UI_LINK_OPCODE_HANDLER;
        msg_param->opcode_handler.opcode = opcode_handler->opcode;
		msg_param->opcode_handler.param_length = opcode_handler->param_length;
		msg_param->opcode_handler.dest_address = opcode_handler->dest_address;
		msg_param->opcode_handler.source_address = opcode_handler->source_address;
		memcpy(msg_param->opcode_handler.param, opcode_handler->param, opcode_handler->param_length);
        GxBus_MessageSend(new_msg);
    }
	return GXCORE_SUCCESS;
}

int app_cec_link_ui_msg_proc(GxMessage *msg)
{
    AppMsg_CecUIControl *ui_msg;

    if(msg == NULL)
    {
        return EVENT_TRANSFER_STOP;
    }

    ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_CecUIControl);
    app_log_debug("ui_msg type:%d\n", ui_msg->type);
    switch(ui_msg->type)
    {
        case CEC_UI_SHOW_MESSAGE:
			app_cec_link_show_osd_message(&ui_msg->opcode_handler);
			break;
        case CEC_UI_LINK_OPCODE_HANDLER:
			app_cec_link_opcode_handler(&ui_msg->opcode_handler);
			break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

#endif
