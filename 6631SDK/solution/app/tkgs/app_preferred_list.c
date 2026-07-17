#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#include "module/tkgs/gxtkgs.h"
#include "app_wnd_system_setting_opt.h"
#include "app_windows.h"

#if TKGS_SUPPORT
/* Private variable------------------------------------------------------- */
#define TKGS_PREFER_NAME_LEN	20

static GxTkgsPreferredList s_PreferredList;
static GxTkgsPreferredList * p_PreferredList;

static uint16_t s_preferredlist_list_choise = 0;
static uint16_t s_preferredlist_list_total = 0;
static uint16_t s_preferredlist_list_choise_bak;

extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern void app_tkgs_set_operatemode_var(GxTkgsOperatingMode mode);
extern void app_tkgs_sort_channels_by_lcn(void);
/* widget macro -------------------------------------------------------- */

#if 0
static int app_tkgs_update_lcn(void)
{
	GxBusPmViewInfo view_info={0};
	GxBusPmViewInfo view_info_bak={0};
	GxTkgsOperatingMode operateMode;
	GxMsgProperty_NodeNumGet prog_num = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};
	uint32_t i,j;

	GxTkgsPrefer	*p_prefer = NULL;
	GxTkgsService *pTkgsService = NULL;
	uint16_t cur_Locator;
	uint16_t updated_count =0;
	uint16_t k= 0;
	AppProgExtInfo prog_ext_info = {0};

	p_prefer = p_PreferredList->prefer;
	p_prefer += s_preferredlist_list_choise;
	// get and sort to total program
	// get total prog
	operateMode = app_tkgs_get_operatemode();
	if(operateMode != GX_TKGS_OFF)
	{
		app_tkgs_set_operatemode_var(GX_TKGS_OFF);
	}

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info_bak = view_info;

	for(k=0;k<2;k++)
	{

		view_info.group_mode = GROUP_MODE_ALL;	//all
		view_info.taxis_mode = 0;
		if(k == 0)
			view_info.stream_type = GXBUS_PM_PROG_TV;
		else
			view_info.stream_type = GXBUS_PM_PROG_RADIO;

		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

		prog_num.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);

		updated_count = 0;
		for(i=0; i<prog_num.node_num; i++)
		{
			node_get.node_type = NODE_PROG;
			node_get.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
			if (GXCORE_SUCCESS != GxBus_PmProgExtInfoGet(node_get.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
			{
				continue;
			}

			cur_Locator = prog_ext_info.locator_id;
			pTkgsService = p_prefer->list;
			if(cur_Locator == TKGS_INVALID_LOCATOR)
			{
				continue;
			}

			for(j=0;j<p_prefer->serviceNum;j++)
			{
				if(pTkgsService->locator == cur_Locator)
				{
					updated_count ++;
					if(pTkgsService->lcn != prog_ext_info.logicnum)
					{
						prog_ext_info.logicnum = pTkgsService->lcn;
						//GxBus_PmProgInfoModify(&node_get.prog_data);
						GxBus_PmProgExtInfoAdd(node_get.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info);
					}
					break;
				}

				pTkgsService ++;

			}

			if(j == p_prefer->serviceNum)
			{
				prog_ext_info.logicnum = TKGS_INVALID_LCN;
				//GxBus_PmProgInfoModify(&node_get.prog_data);
				GxBus_PmProgExtInfoAdd(node_get.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info);
			}

			if(p_prefer->serviceNum <= updated_count)
			{
				break;
			}
		}
	}

	GxBus_PmSync(GXBUS_PM_SYNC_PROG);
	GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);

	app_tkgs_set_operatemode_var(operateMode);
	view_info_bak.group_mode = GROUP_MODE_ALL;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_bak));

	app_tkgs_sort_channels_by_lcn();//2014.09.23 add

	return 0;
}
#endif

status_t _preferredlist_SetPreferredListPos(void)
{
	GxTkgsPrefer	*p_prefer = NULL;
	if(s_preferredlist_list_choise == s_preferredlist_list_choise_bak)
	{
		return GXCORE_ERROR;
	}

	if(s_preferredlist_list_choise>= s_PreferredList.listNum)
	{
		return GXCORE_ERROR;
	}

	p_prefer = s_PreferredList.prefer;
	p_prefer += s_preferredlist_list_choise;
	//app_log_debug("s_preferredlist_list_choise = %d,name= %s\n",s_preferredlist_list_choise,p_prefer->name);
	app_send_msg_exec(GXMSG_TKGS_SET_PREFERRED_LIST,p_prefer->name);

	return GXCORE_SUCCESS;
}

status_t _preferredlist_GetPreferredListPos(void)
{
	ubyte64 preferred_list_name = {0};
	uint32_t number = 0;
	GxTkgsPrefer	*p_prefer = NULL;

	//app_log_debug("func=%s,line= %d\n",__FUNCTION__,__LINE__);
	app_send_msg_exec(GXMSG_TKGS_GET_PREFERRED_LIST,preferred_list_name);

	s_preferredlist_list_choise = 0;
	if((s_PreferredList.listNum== 0)||(s_PreferredList.listNum <0))
	{
		return GXCORE_ERROR;
	}
	p_prefer = s_PreferredList.prefer;
	while(number<s_PreferredList.listNum)
	{
		//app_log_debug("number = %d,name= %s,selectName=%s\n",number,p_prefer->name,preferred_list_name);
		if(0== strcasecmp(p_prefer->name,preferred_list_name))
		{
			s_preferredlist_list_choise = number;
			break;
		}
		else
		{
			p_prefer ++;
		}
		number ++;
	}
	return GXCORE_SUCCESS;

}


void _preferredlist_getPrefferredLists(void)
{
	s_PreferredList.listNum = 0;
	s_PreferredList.prefer = NULL;
	p_PreferredList = &s_PreferredList;
	app_send_msg_exec(GXMSG_TKGS_GET_PREFERRED_LISTS,p_PreferredList);
}



SIGNAL_HANDLER int app_preferred_list_create(GuiWidget *widget, void *usrdata)
{

	// add in 20120702
	uint32_t cur_sel;
	g_AppFullArb.timer_stop();
	//app_log_debug("func=%s,line= %d\n",__FUNCTION__,__LINE__);

	_preferredlist_getPrefferredLists();
	_preferredlist_GetPreferredListPos();
	s_preferredlist_list_choise_bak = s_preferredlist_list_choise;
	s_preferredlist_list_total = p_PreferredList->listNum;

	if(s_preferredlist_list_total > 0)
	{
		cur_sel = s_preferredlist_list_choise;
		GUI_SetProperty("list_preferred_list", "update_all", NULL);
		GUI_SetProperty("list_preferred_list", "select", &cur_sel);
	}
	//app_log_debug("func=%s,line= %d\n",__FUNCTION__,__LINE__);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_preferred_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int app_preferred_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_Event *event = NULL;
	//app_log_debug("func=%s,line= %d\n",__FUNCTION__,__LINE__);

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{

			case VK_BOOK_TRIGGER:
				app_system_reset_unfocus_image();
				GUI_EndDialog("after wnd_full_screen");
				break;

        		case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
				{
					ret = EVENT_TRANSFER_STOP;
				}
				break;
			case STBK_MENU:
			case STBK_EXIT:
				_preferredlist_SetPreferredListPos();
				ret = EVENT_TRANSFER_STOP;
				GUI_EndDialog(WND_PREFERRED_LIST);
				break;
			default:
				break;
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_preferred_list_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_preferred_list_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_preferred_list_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_preferred_list_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_preferred_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	_preferredlist_getPrefferredLists();
	return p_PreferredList->listNum;
}

SIGNAL_HANDLER int app_preferred_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	#define MAX_LEN	30
	ListItemPara* item = NULL;
	GxTkgsPrefer	*p_prefer = NULL;
	char * string;
	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: choice
	item->x_offset = 0;
	if (s_preferredlist_list_choise != item->sel)//ID_UNEXIST
	{
		item->image = NULL;
	}
	else
	{
		item->image = "s_choice";
	}
	item->string = NULL;
	p_prefer = p_PreferredList->prefer;
	p_prefer += item->sel;
	//col-1: freq
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	string = p_prefer->name;
	item->string = string;

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_preferred_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	int cur_sel = 0;

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
					GUI_GetProperty("list_preferred_list", "select", &cur_sel);
					//app_log_debug("sel= %d,choise=%d\n",cur_sel,s_preferredlist_list_choise);
					if(cur_sel != s_preferredlist_list_choise)
					{
						s_preferredlist_list_choise = cur_sel;
						GUI_SetProperty("list_preferred_list","update_all",NULL);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}

				case STBK_UP:
					GUI_GetProperty("list_preferred_list", "select", &cur_sel);
					if((cur_sel == 0)&&(s_preferredlist_list_total >0))
					{
						cur_sel = p_PreferredList->listNum -1;
						GUI_SetProperty("list_preferred_list", "select", &cur_sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;

				case STBK_DOWN:

					GUI_GetProperty("list_preferred_list", "select", &cur_sel);
					if(cur_sel == (s_preferredlist_list_total - 1))
					{
						cur_sel = 0;
						GUI_SetProperty("list_preferred_list", "select", &cur_sel);
						ret = EVENT_TRANSFER_STOP;
					}

					break;
				case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty("list_preferred_list", "select", &sel);
					if (sel == 0)
					{
						sel = app_preferred_list_list_get_total(NULL,NULL)-1;
						GUI_SetProperty("list_preferred_list", "select", &sel);
						ret =   EVENT_TRANSFER_STOP;
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON;
					}

				}
					break;
				case STBK_PAGE_DOWN:
				{
					uint32_t sel;
					GUI_GetProperty("list_preferred_list", "select", &sel);
					if (app_preferred_list_list_get_total(NULL,NULL)== sel+1)
					{
						sel = 0;
						GUI_SetProperty("list_preferred_list", "select", &sel);
						ret =  EVENT_TRANSFER_STOP;
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON;
					}
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



// create dialog
int app_preferred_list_menu_exec(void)
{
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PREFERRED_LIST))
	{
		GUI_EndDialog(WND_PREFERRED_LIST);
	}
	GUI_CreateDialog(WND_PREFERRED_LIST);

	return 0;
}
#endif
