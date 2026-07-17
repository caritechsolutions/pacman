#include "app.h"
#include "app_module.h"

#if CI_SUPPORT

#define BTN_SLOT_1 "btn_ci_slot1"
#define BTN_SLOT_2 "btn_ci_slot2"
#define BTN_SLOT_3 "btn_ci_slot3"
#define BTN_SLOT_4 "btn_ci_slot4"

#define CIM_XML "dvb_ci/wnd_cim.xml"
#define WND_CIM "wnd_cim"
#define BTN_WAIT_INFO "btn_ci_wait_info"
#define TXT_CI_MAIN_TITLE "txt_ci_title"
#define TXT_CI_SUB_TITLE "txt_ci_subtitle"
#define TXT_CI_BOTTOM "txt_ci_bottom"
#define LIST_CI_ENTRIES "listview_ci_entries"
#define SRCOLL_CI "list_scroll_ci"

#define CI_PWD_XML "dvb_ci/wnd_ci_pwd.xml"
#define WND_CI_PWD "wnd_ci_pwd"
#define TXT_PWD_TITLE "txt_ci_pwd_title"
#define EDIT_PWD "edit_ci_pwd_input"

#define STR_SLOT_NO_CARD "No Card"
#define STR_SLOT1_NO_CARD "Slot 1: No Card"
#define STR_SLOT_INITIALIZING "Initializing"

#define MAX_LIST_ENTRIES 7
#define MAX_TITLE_LEN 50
#define MS_IMM_WAIT_TIMEOUT 7000

extern ci_ops ci;

static char *s_slot_btn[SOLT_TOTAL] =
{
	BTN_SLOT_1,
	//BTN_SLOT_2,
	//BTN_SLOT_3,
	//BTN_SLOT_4,
};


typedef struct
{
	entry_state s_slot_state;
	char s_slot_str[MAX_TITLE_LEN];
}SlotInfo;

static SlotInfo s_slot_info[SOLT_TOTAL] =
{
	{ENTRY_NO_CARD, STR_SLOT1_NO_CARD}
};

static enum
{
	PWD_DIALOG_OK = 0,
	PWD_DIALOG_CANCLE,
	PWD_DIALOG_NONE
}s_pwd_dlg_ret = PWD_DIALOG_NONE;

static ci_slot s_slot_sel = CI_SLOT_0;
static ci_menu *s_ci_menu = NULL;
static ci_enquiry *s_ci_enquiry = NULL;
static event_list* slot_check_state_timer = NULL;
static event_list* ci_check_state_timer = NULL;
static event_list *ci_wait_state_timer  = NULL;

static status_t app_ci_creat_pwd_dlg(const char *title, int pwdlen, bool intaglio, char **resbuf);

static void app_show_ci_slot(void)
{
	uint8_t i = 0;
	for(i = 0; i < SOLT_TOTAL; i++)
	{
		GUI_SetProperty(s_slot_btn[i], "state", "show");
	}
}

static int app_ci_check_slot_state(void* usrdat)
{

	slot_entry *slot_state = NULL;
	const char *str = NULL;

	slot_state = ci.cim_state_change();

	if((slot_state != NULL)&&(SOLT_TOTAL > slot_state->slot) /*&& (s_slot_info[slot_state->slot].s_slot_state != slot_state->state)*/)
	{
		switch(slot_state->state)
		{
			case ENTRY_NO_CARD:
				str = STR_SLOT_NO_CARD;
				break;

			case ENTRY_INITIALIZING:
				str = STR_SLOT_INITIALIZING;
				break;

			case ENTRY_ESTABLISHED:
			case ENTRY_WAIT:
			case ENTRY_TIMEOUT:
				str = ci.get_name(slot_state->slot);
				break;

			default:
				str = STR_SLOT_NO_CARD;
				break;
		}

		memset(s_slot_info[slot_state->slot].s_slot_str, 0, MAX_TITLE_LEN);
		sprintf(s_slot_info[slot_state->slot].s_slot_str, "Slot %d: ", slot_state->slot+1);
		strcat(s_slot_info[slot_state->slot].s_slot_str, str);

		GUI_SetProperty(s_slot_btn[slot_state->slot], "string", s_slot_info[slot_state->slot].s_slot_str);

		s_slot_info[slot_state->slot].s_slot_state = slot_state->state;

	}

	return 0;
}

SIGNAL_HANDLER int app_ci_slot_create(const char* widgetname, void *usrdata)
{
	int i =0;

	for(i =0; i < SOLT_TOTAL; i++)
	{
		GUI_SetProperty(s_slot_btn[i], "string", s_slot_info[i].s_slot_str);
	}

	if(0 != reset_timer(slot_check_state_timer))
	{
		slot_check_state_timer = create_timer(app_ci_check_slot_state, 500, NULL, TIMER_REPEAT);
	}

	app_show_ci_slot();
	GUI_SetFocusWidget(s_slot_btn[s_slot_sel]);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ci_slot_destroy(const char* widgetname, void *usrdata)
{
	if (slot_check_state_timer != NULL)
	{
		remove_timer(slot_check_state_timer);
		slot_check_state_timer = NULL;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ci_slot_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
				(CI_SLOT_0 == s_slot_sel ) ? (s_slot_sel = SOLT_TOTAL-1) : (s_slot_sel--);
				GUI_SetFocusWidget(s_slot_btn[s_slot_sel]);
				break;


			case STBK_DOWN:
				(SOLT_TOTAL-1 == s_slot_sel ) ? (s_slot_sel = CI_SLOT_0) : (s_slot_sel++);
				GUI_SetFocusWidget(s_slot_btn[s_slot_sel]);
				break;

			case STBK_OK:
				if(s_slot_info[s_slot_sel].s_slot_state >= /*ENTRY_ESTABLISHED*/ENTRY_INITIALIZING)
				{
				     static char flag = 0;
				     if(flag == 0)
				     {
						GUI_LinkDialog(WND_CIM, CIM_XML);
						flag = 1;
					 }

					 GUI_CreateDialog(WND_CIM);
				}
				break;

			case STBK_EXIT:
				ret = EVENT_TRANSFER_KEEPON;
				break;

			default:
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_ci_slot_got_focus(const char* widgetname, void *usrdata)
{
	if(0 != reset_timer(slot_check_state_timer))
	{
		slot_check_state_timer = create_timer(app_ci_check_slot_state, 500, NULL, TIMER_REPEAT);
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_ci_slot_lost_focus(const char* widgetname, void *usrdata)
{
	if (slot_check_state_timer != NULL)
	{
		timer_stop(slot_check_state_timer);
	}
	return EVENT_TRANSFER_STOP;
}

static int app_end_cim_dlg_timer(void* usrdat)
{
	GUI_EndDialog(WND_CIM);
	return 0;
}

static void app_ci_wait_state_begin(void)
{
	GUI_SetProperty(BTN_WAIT_INFO, "state", "show");
	if(0 != reset_timer(ci_wait_state_timer))
	{
		ci_wait_state_timer = create_timer(app_end_cim_dlg_timer, MS_IMM_WAIT_TIMEOUT, NULL, TIMER_ONCE);
	}
}

static void app_ci_wait_state_over(void)
{
	if (ci_wait_state_timer != NULL)
	{
		timer_stop(ci_wait_state_timer);
	}
	GUI_SetProperty(BTN_WAIT_INFO, "state", "hide");
}

static void app_ci_fill_menu(void)
{
	GUI_SetProperty(TXT_CI_MAIN_TITLE, "string", (void *)(s_ci_menu->title_text));
	GUI_SetProperty(TXT_CI_SUB_TITLE, "string", (void *)(s_ci_menu->subtitle_text));
	GUI_SetProperty(TXT_CI_BOTTOM, "string", (void *)(s_ci_menu->bottom_text));
	GUI_SetProperty(LIST_CI_ENTRIES, "update_all", NULL);
}

static int app_ci_check_mode_state(void* usrdat)
{
	mmi_state *state = NULL;
	char *pPasswd = NULL;
	status_t ret = GXCORE_ERROR;

	state = ci.mmi_mode_change();
	if(state == NULL)
	{
		return 0;
	}
	switch(*state)
	{
		case CI_MMI_EXIT:
			app_ci_wait_state_over();
			GUI_EndDialog(WND_CIM);
			break;

		case CI_MMI_MENU:
		case CI_MMI_LIST:
			s_ci_menu = ci.get_menu();
			if(NULL != s_ci_menu)
			{
				app_ci_wait_state_over();
				app_ci_fill_menu();
			}
			break;

		case CI_MMI_ENQ:
			s_ci_enquiry= ci.get_enquiry();
			if(NULL != s_ci_enquiry)
			{
				app_ci_wait_state_over();
				ret =app_ci_creat_pwd_dlg(s_ci_enquiry->text, s_ci_enquiry->expected_length
												,s_ci_enquiry->blind, &pPasswd);
				if(GXCORE_SUCCESS == ret)
				{
					s_ci_enquiry->reply(pPasswd, s_ci_enquiry->expected_length);
					free(pPasswd);
					pPasswd = NULL;
				}
				else
				{
					s_ci_enquiry->cancel();
				}
				app_ci_wait_state_begin();
			}
			break;

		case CI_MMI_DELAY:
			break;

		default:
			break;

	}

	return 0;
}

SIGNAL_HANDLER int app_cim_create(const char* widgetname, void *usrdata)
{
	s_ci_menu = NULL;
	s_ci_enquiry = NULL;

	app_ci_wait_state_begin();

	ci.mmi_open(s_slot_sel);
	if(0 != reset_timer(ci_check_state_timer))
	{
		ci_check_state_timer = create_timer(app_ci_check_mode_state, 300, NULL, TIMER_REPEAT);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cim_destroy(const char* widgetname, void *usrdata)
{
	if (ci_check_state_timer != NULL)
	{
		remove_timer(ci_check_state_timer);
		ci_check_state_timer = NULL;
	}

	if (ci_wait_state_timer!= NULL)
	{
		remove_timer(ci_wait_state_timer);
		ci_wait_state_timer = NULL;
	}

	s_ci_menu = NULL;
	s_ci_enquiry = NULL;

	ci.mmi_close(s_slot_sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cim_got_focus(const char* widgetname, void *usrdata)
{
	if(0 != reset_timer(ci_check_state_timer))
	{
		ci_check_state_timer = create_timer(app_ci_check_mode_state, 300, NULL, TIMER_REPEAT);
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_cim_lost_focus(const char* widgetname, void *usrdata)
{
	if (ci_check_state_timer != NULL)
	{
		timer_stop(ci_check_state_timer);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ci_entries_get_total(const char* widgetname, void *usrdata)
{
	int count = 0;

	(s_ci_menu == NULL) ? (count = 0) : (count = s_ci_menu->entries_num);

	if(count > MAX_LIST_ENTRIES)
	{
		GUI_SetProperty(SRCOLL_CI, "format", "scroll_show");
	}
	else
	{
		GUI_SetProperty(SRCOLL_CI, "format", "scroll_hide");
	}

	return count;
}

SIGNAL_HANDLER int app_ci_entries_get_data(const char* widgetname, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if((NULL == item) || (NULL == s_ci_menu) ||(NULL == s_ci_menu->entries_text[item->sel]))
	{
		return EVENT_TRANSFER_KEEPON;
	}

	item->string = s_ci_menu->entries_text[item->sel];

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ci_entries_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int sel = -1;
	int ret = EVENT_TRANSFER_KEEPON;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{

			case STBK_OK:
				app_ci_wait_state_begin();
				GUI_GetProperty(LIST_CI_ENTRIES, "select", &sel);
				if(NULL != s_ci_menu)
				{
					s_ci_menu ->select(sel);
				}
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_EXIT:
				if(NULL != s_ci_menu)
				{
					s_ci_menu ->cancel();
				}
				app_ci_wait_state_begin();
				ret = EVENT_TRANSFER_STOP;
				break;

			default:
				break;
		}
	}
	return ret;
}

/****need free the resbuf by self if the return value is GXCORE_SUCCUSS*****/
static status_t app_ci_creat_pwd_dlg(const char *title, int pwdlen, bool intaglio, char **resbuf)
{
	char *retstr = NULL;
	char *editstr = NULL;
	int retlen = 0;
	status_t ret = GXCORE_ERROR;
	char lenthbuf[10];
	static char flag = 0;
    	s_pwd_dlg_ret = PWD_DIALOG_NONE;

	if(flag == 0)
	{
		GUI_LinkDialog(WND_CI_PWD, CI_PWD_XML);
		flag = 1;
	}
    	GUI_CreateDialog(WND_CI_PWD);

    	GUI_SetProperty(TXT_PWD_TITLE, "string", (void*)title);

	memset(lenthbuf, 0, sizeof(lenthbuf));
    	snprintf(lenthbuf, sizeof(lenthbuf), "%d", pwdlen);
    	GUI_SetProperty(EDIT_PWD, "maxlen", lenthbuf);

	if(true == intaglio)
	{
		GUI_SetProperty(EDIT_PWD, "default_intaglio", "-");
    		GUI_SetProperty(EDIT_PWD, "intaglio", "*");
    	}
   #if 0
    	while(PWD_DIALOG_NONE == s_pwd_dlg_ret)
    	{
        	GUI_Exec();
        	GxCore_ThreadDelay(50);
    	}
	#endif
		app_msg_destroy(g_app_msg_self);

		while(PWD_DIALOG_NONE == s_pwd_dlg_ret)
	    {
	        GUI_Loop();
	        GxCore_ThreadDelay(50);
	    }
		GUI_StartSchedule();
		app_msg_init(g_app_msg_self);

    	if(PWD_DIALOG_OK == s_pwd_dlg_ret)
    	{
    		GUI_GetProperty(EDIT_PWD, "string", &editstr);
    		if(NULL != editstr)
    		{
    			retlen = strlen(editstr);
    			if((retstr = (char *)malloc(retlen+1)) !=NULL)
    			{
    				strncpy(retstr, editstr, retlen);
    				retstr[retlen] = '\0';
    				*resbuf = retstr;
    				ret = GXCORE_SUCCESS;
    			}
    		}
    	}

    	GUI_EndDialog(WND_CI_PWD);
    	s_pwd_dlg_ret = PWD_DIALOG_NONE;

    	return ret;
}

SIGNAL_HANDLER int app_ci_pwd_dlg_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
				s_pwd_dlg_ret = PWD_DIALOG_OK;
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_EXIT:
				s_pwd_dlg_ret = PWD_DIALOG_CANCLE;
				ret = EVENT_TRANSFER_STOP;
				break;
			default:
				break;
		}
	}
	return ret;
}

void app_dvb_ci_menu_exec(void)
{
#define WND_CI_SLOT                      "wnd_ci_slot"
#define CI_SLOT_XML                      "dvb_ci/wnd_ci_slot.xml"
   static char flag = 0;
   if(flag == 0)
   {
      GUI_LinkDialog(WND_CI_SLOT, CI_SLOT_XML);
      flag = 1;
   }


   GUI_CreateDialog(WND_CI_SLOT);

   return;
}
#endif

