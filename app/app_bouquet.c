#include "module/app_bat.h"
#include "app_windows.h"
#include "channel_common.h"
#include "app_channel_info.h"

#if BOUQUET_SUPPORT
#define BOUQUET_TXT_GROUP      "txt_group_bouquet"
#define BOUQUET_IMG_ARROW_L    "img_bouquet_left_arrow"
#define BOUQUET_IMG_ARROW_R    "img_bouquet_right_arrow"
#define BOUQUET_TXT_TP         "text_bouquet_tp"
#define BOUQUET_LV_LIST    	   "listview_bouquet_list"
#define BOUQUET_IMG_BOTTOM 	   "img_bouquet_bottom"
#define BOUQUET_IMG_BODY       "img_bouquet_body"

typedef enum
{
	BOUQUET_MODE_TV,
	BOUQUET_MODE_RADIO,
    BOUQUET_MODE_GROUP,
    BOUQUET_MODE_ITEM,
    BOUQUET_MODE_TOTAL
}BouquetMode;

BouquetMode  g_bouquet_mode = 0;

static int32_t bouquet_index = 0;
static int32_t current_bouqet_id = 0;

static int bouquet_item_num = 0;
static int bouquet_num = 0;
static int _bouquet_list_get_total(void);
/* widget macro -------------------------------------------------------- */

static inline void _bouquet_show_group(char* str)
{
    if (str == NULL)
    {
        GUI_SetProperty(BOUQUET_IMG_BOTTOM, "state", "show");
        GUI_SetProperty(BOUQUET_TXT_TP,     "state", "show");

        GUI_SetProperty(BOUQUET_IMG_ARROW_L,"state", "show");
        GUI_SetProperty(BOUQUET_IMG_ARROW_R,"state", "show");
    }
    else
    {
     //   GUI_SetProperty(BOUQUET_IMG_BODY,   "state", "hide");
        GUI_SetProperty(BOUQUET_IMG_BODY,   "state", "show");
		GUI_SetProperty(BOUQUET_IMG_BOTTOM, "state", "hide");
        GUI_SetProperty(BOUQUET_TXT_TP,     "state", "hide");
        GUI_SetProperty(BOUQUET_IMG_ARROW_L,"state", "hide");
        GUI_SetProperty(BOUQUET_IMG_ARROW_R,"state", "hide");

        GUI_SetProperty(BOUQUET_TXT_GROUP,  "string", str);
    }
}

static void app_bouquet_get_data_font(void)
{
    int select = 0, i = 0;
    int index = 0;
    GuiWidget *value = NULL;

    char *item_name[12] = {
        "listitem_bouquet_1",
        "listitem_bouquet_2",
        "listitem_bouquet_3",
        "listitem_bouquet_4",
        "listitem_bouquet_5",
        "listitem_bouquet_6",
        "listitem_bouquet_7",
        "listitem_bouquet_8",
        "listitem_bouquet_9",
        "listitem_bouquet_10"
    };

    for(i = 0; i <10; i++)
    {
        GUI_SetProperty(item_name[i], "font", "roboto");
    }


    for(index = 0; index<10;index++)
    {
        GUI_GetProperty(BOUQUET_LV_LIST, "focus_widget", &value);

        if((value&&(value->name!=NULL)) && (0 == strcmp(value->name, item_name[index])))
        {
            select = index;
            break;
        }

    }

    GUI_SetProperty(item_name[select], "font", "font_prog");//font_mid
    return;
}

static void _bouquet_list_change(void)
{
#define TP_LEN	50
    uint32_t sel=0;
    static GxMsgProperty_NodeByPosGet ProgNode;
    char buffer[TP_LEN];

    memset(buffer, 0, sizeof(buffer));
    if(_bouquet_list_get_total() !=0 ){
        GUI_GetProperty(BOUQUET_LV_LIST, "select", &sel);
        //get prog node
        ProgNode.node_type = NODE_PROG;
        ProgNode.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

        channel_prog_tp_info_str_get(&ProgNode.prog_data, buffer, TP_LEN);
    }else{
        strcpy(buffer, " ");
    }
    GUI_SetProperty(BOUQUET_TXT_TP,"string",buffer);

    if( APP_IS_NOT_THEME_CLASSIC_DARK ){
        app_bouquet_get_data_font();
    }
}
static int _bouquet_list_get_total(void)
{
	uint32_t total = 0;
	if(g_bouquet_mode == BOUQUET_MODE_GROUP)
	{
		 bouquet_num = (app_get_bouquet_num() + 2);
		 total = bouquet_num;
	}
	else if(g_bouquet_mode == BOUQUET_MODE_ITEM)
	{
		bouquet_item_num = app_bouquet_list_get_num(bouquet_index,current_bouqet_id);
		total = bouquet_item_num;
	}
	else
	{
        GxBusPmViewInfo sysinfo = {0};
        GxBus_PmViewInfoGet(&sysinfo);
        sysinfo.group_mode = GROUP_MODE_ALL;
        if(g_bouquet_mode == BOUQUET_MODE_RADIO)
            sysinfo.stream_type = GXBUS_PM_PROG_RADIO;
        else if(g_bouquet_mode == BOUQUET_MODE_TV)
            sysinfo.stream_type = GXBUS_PM_PROG_TV;
        total = GxBus_PmProgNumGetByViewInfo(&sysinfo);
	}

	return total;
}
static status_t _bouquet_group_get_data(void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
    ListItemPara* item = NULL;
	static char buffer[CHANNEL_NUM_LEN];

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 0;
    item->image = NULL;
	memset(buffer,0,CHANNEL_NUM_LEN);
	sprintf((void*)buffer, "%0d",item->sel+1);
    item->string = buffer;

    //col-1: bouquet name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

	if(item->sel == 0)
	{
		item->string = "ALL TV CHANNEL";
	}
	else if(item->sel == 1)
	{
		item->string = "ALL RADIO CHANNEL";
	}
	else
	{
		item->string = (char*)app_get_bouquet_name(item->sel-2,NULL);
	}
    return GXCORE_SUCCESS;
}

static status_t _bouquet_prog_list_get_data(void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
	static GxBusPmDataProg prog;
    ListItemPara* item = NULL;
    static char buffer[CHANNEL_NUM_LEN];
	int get_pos = 0;
	int ret = 0;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer, 0, CHANNEL_NUM_LEN);

	if(g_bouquet_mode == BOUQUET_MODE_ITEM)
	{
		get_pos = app_bouquet_list_get_prog_id(bouquet_index,item->sel,current_bouqet_id);
		if(get_pos == -1)
		{
			app_log_error("error ,can't find this item\n");
			return GXCORE_ERROR;
		}
		ret = GxBus_PmProgGetById(get_pos,&prog);
		if(ret < 0)
		{
			app_log_error("%s %d get prog by id failed\n",__func__,__LINE__);
			return 0;
		}
	}
    else //tv or radio
    {
        GxBusPmViewInfo sysinfo = {0};
        GxBus_PmViewInfoGet(&sysinfo);
        sysinfo.group_mode = GROUP_MODE_ALL;
        if(g_bouquet_mode == BOUQUET_MODE_RADIO)
            sysinfo.stream_type = GXBUS_PM_PROG_RADIO;
        else if(g_bouquet_mode == BOUQUET_MODE_TV)
            sysinfo.stream_type = GXBUS_PM_PROG_TV;
        GxBus_PmViewInfoMemSet(&sysinfo);
        GxBus_PmProgGetByPos(item->sel,1,&prog);
        GxBus_PmViewInfoMemClear();
    }
    sprintf(buffer, "%d", (item->sel+1));
    item->string = buffer;

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

	if(g_bouquet_mode == BOUQUET_MODE_ITEM)
	{
		item->string  = (char*)prog.prog_name;
	}
	else
	{
		item->string = (char*)prog.prog_name;
	}

    //col-2: $
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (prog.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        item->image = "s_icon_money";
    else
        item->image = NULL;

    //col-3: lock
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (prog.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        item->image = "s_icon_lock";
    else
        item->image = NULL;

    //col-4: hd
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;

    if ((g_AppPvrOps.state == PVR_RECORD || g_AppPvrOps.state == PVR_TMS_AND_REC) && prog.id == g_AppPvrOps.env.prog_id)
    {
        item->image = "s_ts_red";
    }
    else
    {
        if (prog.definition == GXBUS_PM_PROG_HD)
        {
            item->image = "s_icon_hd2";
        }
        else
        {
            item->image = NULL;
        }
    }

    return GXCORE_SUCCESS;
}

static bool _bouquet_prog_find(int32_t prog_id, int32_t *prog_pos)
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
                    app_log_info("[%s] Find program in target viewinfo in [Pos:%d/Total:%d]\n", __FILE__, node.pos, nodeNum.node_num);
                    if(prog_pos != NULL)
                    {
                        *prog_pos = pos;
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

int _bouquet_play_prog(void)
{
	uint32_t sel = 0;
    uint32_t active_sel = 0;
	int32_t prog_id = 0;
	int32_t prog_pos = -1;
	int32_t play_pos = -1;
	GxBusPmViewInfo sysinfo = {0};
    GxBusPmDataProg prog = {0};


	GUI_GetProperty(BOUQUET_LV_LIST, "select", (void*)&sel);
    GUI_GetProperty(BOUQUET_LV_LIST,"active",&active_sel);
    if(sel == active_sel)
    {
        GUI_EndDialog(WND_BOUQUET);
        GUI_SetProperty(WND_BOUQUET,"draw_now",NULL);
        app_channel_info_create_dialog();
        return 0;
    }
	GxBus_PmViewInfoGet(&sysinfo);
    GUI_SetProperty(BOUQUET_LV_LIST, "active", &sel);
	if(g_bouquet_mode == BOUQUET_MODE_ITEM)
	{
		prog_id = app_bouquet_list_get_prog_id(bouquet_index,sel,current_bouqet_id);
		if(prog_id == -1)
			return 0;
		if(true == _bouquet_prog_find(prog_id, &prog_pos))
		{
			play_pos = prog_pos;
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            g_AppPlayOps.program_play(PLAY_MODE_POINT, play_pos);
            return 0;
		}
		else
		{
            sysinfo.group_mode = GROUP_MODE_ALL;
            GxBus_PmViewInfoMemSet(&sysinfo);
            if(true == _bouquet_prog_find(prog_id, &prog_pos))
            {
                GxBus_PmViewInfoMemClear();
                play_pos = prog_pos;
                g_AppPlayOps.normal_play.view_info.group_mode = sysinfo.group_mode;
                g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                g_AppPlayOps.program_play(PLAY_MODE_POINT, play_pos);
                return 0;
            }
            else
            {
                if(sysinfo.stream_type == GXBUS_PM_PROG_TV)
                    sysinfo.stream_type = GXBUS_PM_PROG_RADIO;
                else
                    sysinfo.stream_type = GXBUS_PM_PROG_TV;
                GxBus_PmViewInfoMemSet(&sysinfo);
                if(true == _bouquet_prog_find(prog_id, &prog_pos))
                {
                    GxBus_PmViewInfoMemClear();
                    play_pos = prog_pos;
                    g_AppPlayOps.normal_play.view_info.group_mode = sysinfo.group_mode;
                    g_AppPlayOps.normal_play.view_info.stream_type = sysinfo.stream_type;
                    if(sysinfo.stream_type == GXBUS_PM_PROG_TV)
                        g_AppFullArb.state.tv = STATE_ON;
                    else
                        g_AppFullArb.state.tv = STATE_OFF;
                    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                    g_AppFullArb.state.pause = STATE_OFF;
                    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                    g_AppPlayOps.program_play(PLAY_MODE_POINT, play_pos);
                    return 0;
                }
                GxBus_PmViewInfoMemClear();
                return 0;
            }
			return 0;
		}
	}
    else if(g_bouquet_mode == BOUQUET_MODE_TV || g_bouquet_mode == BOUQUET_MODE_RADIO)
    {
        if(g_bouquet_mode == BOUQUET_MODE_TV)
        {
            sysinfo.group_mode = GROUP_MODE_ALL;
            if( sysinfo.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_PmViewInfoMemSet(&sysinfo);
                GxBus_PmProgGetByPos(sel,1,&prog);
                prog_id = prog.id;
                GxBus_PmViewInfoMemClear();
                if(true == _bouquet_prog_find(prog_id, &prog_pos))
                {
                    play_pos = prog_pos;
                }
                else
                {
                    g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
                    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                    play_pos = sel;
                }
            }
            else
            {
                g_AppFullArb.state.tv = STATE_ON;
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
                g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                play_pos = sel;
            }
        }
        else
        {
            sysinfo.group_mode = GROUP_MODE_ALL;
            if( sysinfo.stream_type == GXBUS_PM_PROG_RADIO)
            {
                GxBus_PmViewInfoMemSet(&sysinfo);
                GxBus_PmProgGetByPos(sel,1,&prog);
                prog_id = prog.id;
                GxBus_PmViewInfoMemClear();
                if(true == _bouquet_prog_find(prog_id, &prog_pos))
                {
                    play_pos = prog_pos;
                }
                else
                {
                    g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
                    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                    play_pos = sel;
                }
            }
            else
            {
                g_AppFullArb.state.tv = STATE_OFF;
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
                g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                play_pos = sel;
            }
        }
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppPlayOps.program_play(PLAY_MODE_POINT, play_pos);
        return 0;
    }
    return 0;
}

void app_bouquet_backup_refresh(int type, uint32_t pos)
{
	int sel = 0;

	if(g_bouquet_mode == BOUQUET_MODE_GROUP)
	{
		if(type == 2)
		{
			GUI_GetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
			GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
			if((sel >= bouquet_num) && (bouquet_num != 0))
			{
				sel = 0;
				GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
			}
			else
			{
				if(bouquet_num != 0)
				{
					GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
				}
			}
		}

	}
	else if(g_bouquet_mode == BOUQUET_MODE_ITEM)
	{
		GUI_GetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
		GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
		if((sel >= bouquet_item_num) && (bouquet_item_num != 0))
		{
			sel = 0;
			GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
		}
		else
		{
			if(bouquet_item_num != 0)
			{
				GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&sel);
			}
		}
	}
	else
	{
		if(type == 2)
		{
			return;
		}
		else
		{
			GUI_SetProperty(BOUQUET_LV_LIST, "update_all", NULL);
			GUI_SetProperty(BOUQUET_LV_LIST, "select", &pos);
		}
	}
}

static void _bouquet_list_set_select(BouquetMode mode, int32_t bouquet_index)
{
    uint32_t value = 0;
    uint32_t prog_id;
	GxBusPmViewInfo sysinfo = {0};
    if( mode == BOUQUET_MODE_TV || mode == BOUQUET_MODE_RADIO )
    {
        GxBus_PmViewInfoGet(&sysinfo);
        sysinfo.group_mode = GROUP_MODE_ALL;
        if(mode == BOUQUET_MODE_RADIO)
            sysinfo.stream_type = GXBUS_PM_PROG_RADIO;
        else if(mode == BOUQUET_MODE_TV)
            sysinfo.stream_type = GXBUS_PM_PROG_TV;
        GxBus_PmViewInfoMemSet(&sysinfo);
        GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
        if(channel_get_playing_pos_in_group(&value) == TRUE)
        {
            GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&value);
            GUI_SetProperty(BOUQUET_LV_LIST, "active", &value);
        }
        else
            GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&value);
        GxBus_PmViewInfoMemClear();
    }
    else if( mode == BOUQUET_MODE_ITEM )
    {
        GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
            prog_id = g_AppPlayOps.normal_play.view_info.tv_prog_cur;
        else
            prog_id = g_AppPlayOps.normal_play.view_info.radio_prog_cur;
        if (app_bouquet_list_get_pos_by_progid( bouquet_index, prog_id, &value ) == 1)
        {
            GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&value);
            GUI_SetProperty(BOUQUET_LV_LIST, "active", &value);
        }
        else
            GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&value);
        _bouquet_list_change();
    }
}


SIGNAL_HANDLER int app_bouquet_create(GuiWidget *widget, void *usrdata)
{
	g_bouquet_mode = BOUQUET_MODE_GROUP;
	_bouquet_show_group(STR_ID_BOUQUET);

    GUI_SetFocusWidget(BOUQUET_LV_LIST);
    if( APP_IS_NOT_THEME_CLASSIC_DARK ){
        app_bouquet_get_data_font();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bouquet_destroy(GuiWidget *widget, void *usrdata)
{
    GxBusPmViewInfo sysinfo = {0};
	GxMsgProperty_NodeByPosGet node_prog_tmp = {0};

	node_prog_tmp.node_type = NODE_PROG;
	node_prog_tmp.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);

    GxBus_PmViewInfoGet(&sysinfo);
	if(g_AppPlayOps.normal_play.view_info.stream_type != sysinfo.stream_type)
	{
		sysinfo.group_mode = GROUP_MODE_ALL;
		sysinfo.stream_type = g_AppPlayOps.normal_play.view_info.stream_type;
		GxBus_PmViewInfoModify(&sysinfo);
	}
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bouquet_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    uint32_t value = 0;
    GUI_Event *event = NULL;
	GxBusPmViewInfo sysinfo = {0};

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    {
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WND_BOUQUET);
                        GUI_SetInterface("flush", NULL);
                        app_channel_info_create_dialog();
                    }
					break;
                case VK_BOOK_TRIGGER:
                    {
                        GUI_EndDialog(WND_BOUQUET);
                        GUI_SetInterface("flush", NULL);
                        app_channel_info_create_dialog();
                    }
                    break;
				case STBK_F1:
					{
						if(g_bouquet_mode == BOUQUET_MODE_GROUP)
						{
							break;
						}
						else if(g_bouquet_mode == BOUQUET_MODE_TV)
						{
							value = 0;
						}
						else if(g_bouquet_mode == BOUQUET_MODE_RADIO)
						{
							value = 1;
						}
						else if(g_bouquet_mode == BOUQUET_MODE_ITEM)
						{
							value = bouquet_index + 2;
						}
                    	g_bouquet_mode = BOUQUET_MODE_GROUP;
						_bouquet_show_group(STR_ID_BOUQUET);
						GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
						GUI_SetProperty(BOUQUET_LV_LIST,"select",(void*)&value);
						bouquet_index = 0;
						current_bouqet_id = 0;
					}
					break;
				case STBK_LEFT:
					if(g_bouquet_mode == BOUQUET_MODE_TV)
					{
						int num = app_get_bouquet_num();

						if(num > 0)
						{
							char buffer[40]={0};
							bouquet_index = num-1;
							g_bouquet_mode = BOUQUET_MODE_ITEM;
							sprintf(buffer,"%s",app_get_bouquet_name(bouquet_index,&current_bouqet_id));
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string",buffer);
						}
						else
						{
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","Radio");
                            g_bouquet_mode = BOUQUET_MODE_RADIO;
						}
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
					}
					else if(g_bouquet_mode == BOUQUET_MODE_RADIO)
					{
						GUI_SetProperty(BOUQUET_TXT_GROUP,"string","TV");
						g_bouquet_mode = BOUQUET_MODE_TV;
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);

					}
					else if(g_bouquet_mode == BOUQUET_MODE_ITEM)
					{
						char buffer[40]={0};
						bouquet_index--;
						if(bouquet_index < 0)
						{
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","Radio");
							g_bouquet_mode = BOUQUET_MODE_RADIO;
                        }
						else
						{
							sprintf(buffer,"%s",app_get_bouquet_name(bouquet_index,&current_bouqet_id));
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string",buffer);
						}
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
					}
				break;
				case STBK_RIGHT:
					if(g_bouquet_mode == BOUQUET_MODE_TV)
					{
						GUI_SetProperty(BOUQUET_TXT_GROUP,"string","Radio");
						g_bouquet_mode = BOUQUET_MODE_RADIO;
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);

					}
					else if(g_bouquet_mode == BOUQUET_MODE_RADIO)
					{
						char buffer[40] = {0};
						int num = app_get_bouquet_num();
						if(num  > 0)
						{
							g_bouquet_mode = BOUQUET_MODE_ITEM;
							bouquet_index = 0;
							sprintf(buffer,"%s",app_get_bouquet_name(bouquet_index,&current_bouqet_id));
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string",buffer);
						}
						else
						{
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","TV");
							g_bouquet_mode = BOUQUET_MODE_TV;
						}
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
					}
					else if(g_bouquet_mode == BOUQUET_MODE_ITEM)
					{
						int num = app_get_bouquet_num();
						char buffer[40] = {0};
						bouquet_index++;
						if(bouquet_index >= num)
						{
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","TV");
							g_bouquet_mode = BOUQUET_MODE_TV;
						}
						else
						{
							sprintf(buffer,"%s",app_get_bouquet_name(bouquet_index,&current_bouqet_id));
							GUI_SetProperty(BOUQUET_LV_LIST,"update_all",NULL);
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string",buffer);
                        }
                        _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
					}
					break;
				case STBK_OK:
					if(g_bouquet_mode == BOUQUET_MODE_GROUP)
					{
						int sel = 0;
						char buffer[40]={0};
						GUI_GetProperty(BOUQUET_LV_LIST,"select",&sel);
						if(sel == 0)
						{
							g_bouquet_mode = BOUQUET_MODE_TV;
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","TV");
							_bouquet_show_group(NULL);
                            _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
						}
						else if(sel == 1)
						{
							g_bouquet_mode = BOUQUET_MODE_RADIO;
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string","RADIO");
							_bouquet_show_group(NULL);
                            _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
						}
						else
						{
							bouquet_index = sel-2;
							g_bouquet_mode = BOUQUET_MODE_ITEM;
							_bouquet_show_group(NULL);
							sprintf(buffer,"%s",app_get_bouquet_name(bouquet_index,&current_bouqet_id));
							GUI_SetProperty(BOUQUET_TXT_GROUP,"string",buffer);
                            _bouquet_list_set_select(g_bouquet_mode,bouquet_index);
						}
						_bouquet_list_change();
					}
					else
					{
                        uint32_t total = 0;
                        if(g_bouquet_mode == BOUQUET_MODE_ITEM)
                        {
                            total = app_bouquet_list_get_num(bouquet_index,current_bouqet_id);
                        }
                        else if(g_bouquet_mode == BOUQUET_MODE_TV || g_bouquet_mode == BOUQUET_MODE_RADIO)
                        {
                            GxBus_PmViewInfoGet(&sysinfo);
                            sysinfo.group_mode = GROUP_MODE_ALL;
                            if(g_bouquet_mode == BOUQUET_MODE_RADIO)
                                sysinfo.stream_type = GXBUS_PM_PROG_RADIO;
                            else if(g_bouquet_mode == BOUQUET_MODE_TV)
                                sysinfo.stream_type = GXBUS_PM_PROG_TV;
                            total = GxBus_PmProgNumGetByViewInfo(&sysinfo);
                        }
						if(total != 0)
						{
							_bouquet_play_prog();
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

SIGNAL_HANDLER int app_bouquet_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _bouquet_list_get_total();
}

SIGNAL_HANDLER int app_bouquet_list_get_data(GuiWidget *widget, void *usrdata)
{
	if (g_bouquet_mode == BOUQUET_MODE_GROUP)
    {
		_bouquet_group_get_data(usrdata);
    }
    else
    {
		_bouquet_prog_list_get_data(usrdata);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bouquet_list_change(GuiWidget *widget, void *usrdata)
{
	if(g_bouquet_mode != BOUQUET_MODE_GROUP)
	{
		_bouquet_list_change();
	}
    return EVENT_TRANSFER_STOP;
}

#endif

