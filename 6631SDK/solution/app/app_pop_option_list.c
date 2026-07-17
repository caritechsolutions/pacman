#include "app.h"
#include "app_pop.h"
#include "app_windows.h"

#define TEXT_POPLIST_TITILE "text_pop_option_title"
#define LISTVIEW_POPLIST "listview_pop_option"

static enum
{
	POP_LIST_END,
	POP_LIST_START,
}s_pop_list_sate = POP_LIST_END;
static PopList s_pop_list_param;
static int s_select_ret = -1;
static unsigned short s_RetKey = STBK_EXIT;
static char *s_muti_select_arry = NULL;

int app_pop_list_end_dialog(void);

static void _pop_option_id_add(int sel)
{
    if(s_muti_select_arry)
        s_muti_select_arry[sel] = 1;
}
static void _pop_option_id_del(int sel)
{
    if(s_muti_select_arry)
        s_muti_select_arry[sel] = 0;

}
static int _pop_option_id_check(int sel)
{
    if((s_pop_list_param.muti_select != POP_MULTI_SELECT_ENABLE) || (s_muti_select_arry == NULL))
        return -1;
    if(s_muti_select_arry[sel] == 1)
        return 1;
    else
        return 0;
}

SIGNAL_HANDLER int app_pop_option_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(TEXT_POPLIST_TITILE, "string", (void*)(s_pop_list_param.title));
	GUI_SetProperty(LISTVIEW_POPLIST, "select", &s_pop_list_param.sel);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_option_destroy(GuiWidget *widget, void *usrdata)
{
	if(s_muti_select_arry)
	{
		GxCore_Free(s_muti_select_arry);
		s_muti_select_arry = NULL;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_option_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    int ret = 0;

    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_OK:
                GUI_GetProperty(LISTVIEW_POPLIST, "select", &s_select_ret);
                if(s_pop_list_param.muti_select == POP_MULTI_SELECT_ENABLE)
                {
                    ret = _pop_option_id_check(s_select_ret);
                    if(1 == ret) {
                        _pop_option_id_del(s_select_ret);
                        GUI_SetProperty(LISTVIEW_POPLIST, "update_row", &s_select_ret);
                    } else if(0 == ret) {
                        _pop_option_id_add(s_select_ret);
                        GUI_SetProperty(LISTVIEW_POPLIST, "update_row", &s_select_ret);
                    }
                }
                else
                {
                    s_pop_list_sate = POP_LIST_END;
                    s_RetKey = event->key.sym;
                    if(POP_MODE_UNBLOCK == s_pop_list_param.mode)
                    {
                        GUI_EndDialog(WND_POP_OPTION_LIST);
                        if(s_pop_list_param.exit_cb != NULL)
                        {
                            s_pop_list_param.exit_cb(s_select_ret, s_RetKey);
                        }
                    }
                }
                break;

            case STBK_MENU:
            case STBK_EXIT:
                s_select_ret = s_pop_list_param.sel;
                s_RetKey = event->key.sym;
                s_pop_list_sate = POP_LIST_END;
                if(POP_MODE_UNBLOCK == s_pop_list_param.mode)
                {
                    if((s_pop_list_param.muti_select == POP_MULTI_SELECT_ENABLE) && s_pop_list_param.muti_select_arry && s_muti_select_arry)
                    {
                        memcpy(s_pop_list_param.muti_select_arry, s_muti_select_arry, s_pop_list_param.item_num);
                    }
                    GUI_EndDialog(WND_POP_OPTION_LIST);
                    if(s_pop_list_param.exit_cb != NULL)
                    {
                        s_pop_list_param.exit_cb(s_select_ret, s_RetKey);
                    }
                }
                break;

            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_pop_option_list_keypress(GuiWidget *widget, void *usrdata)
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
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_POPLIST, "select", &sel);
					if (sel == 0)
					{
						sel = s_pop_list_param.item_num - 1;
						GUI_SetProperty(LISTVIEW_POPLIST, "select", &sel);
						ret =  EVENT_TRANSFER_STOP;
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
					GUI_GetProperty(LISTVIEW_POPLIST, "select", &sel);
					if (s_pop_list_param.item_num == sel + 1)
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_POPLIST, "select", &sel);
						ret =  EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
				}
				break;
				case STBK_UP:
				case STBK_DOWN:
				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_pop_option_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_pop_list_param.item_num;
}

SIGNAL_HANDLER int app_pop_option_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;
	static char num_buf[10] = {0};

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;

	if(s_pop_list_param.show_num == false)
	{
		item->string = NULL;
	}
	else
	{
		memset(num_buf, 0, sizeof(num_buf));
		sprintf(num_buf, " %d", (item->sel+1));
		item->string = num_buf;
	}

	//col-1:
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_pop_list_param.item_content[item->sel];

	//col-2:
	item = item->next;
	item->x_offset = 0;
	if (_pop_option_id_check(item->sel) == 1)
		item->image = "s_choice";
	else
		item->image = NULL;
	item->string = NULL;

	return EVENT_TRANSFER_STOP;
}

int poplist_create(PopList* pop_list)
{
    int32_t pos_x = 0;
    int32_t pos_y = 0;

    if(pop_list == NULL)
    {
        return -1;
    }

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_OPTION_LIST))
    {
        app_pop_list_end_dialog();
    }
    s_RetKey = STBK_EXIT;
    memcpy(&s_pop_list_param, pop_list, sizeof(PopList));
    if((s_pop_list_param.muti_select == POP_MULTI_SELECT_ENABLE) && s_pop_list_param.muti_select_arry)
    {
        APP_FREE(s_muti_select_arry);
        s_muti_select_arry = (char *)GxCore_Malloc(s_pop_list_param.item_num);
        if(s_muti_select_arry)
            memcpy(s_muti_select_arry, s_pop_list_param.muti_select_arry, s_pop_list_param.item_num);
    }

    GUI_CreateDialog(WND_POP_OPTION_LIST);
    if((s_pop_list_param.pos.x==0)&&(s_pop_list_param.pos.y==0))
    {
        pos_x = APP_WND_POP_OP_LIST_POS_X;
        pos_y = APP_WND_POP_OP_LIST_POS_Y;
    }
    else
    {
        pos_x = s_pop_list_param.pos.x;
        pos_y = s_pop_list_param.pos.y;
    }

    GUI_SetProperty(WND_POP_OPTION_LIST, "move_window_x", &pos_x);
    GUI_SetProperty(WND_POP_OPTION_LIST, "move_window_y", &pos_y);

    if(POP_MODE_BLOCK == s_pop_list_param.mode)
    {
        app_log_error("######## poplist mode is POP_MODE_BLOCK, please modify POP_MODE_UNBLOCK \n");
        s_pop_list_sate = POP_LIST_START;
        app_block_msg_destroy(g_app_msg_self);
        while(s_pop_list_sate == POP_LIST_START)
        {
            GUI_LoopEvent();
            GxCore_ThreadDelay(50);
        }
        GUI_StartSchedule();
        app_block_msg_init(g_app_msg_self);
        if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_POP_OPTION_LIST);
        }
        if(s_pop_list_param.exit_cb != NULL)
        {
            s_pop_list_param.exit_cb(s_select_ret, s_RetKey);
        }
    }
    return s_select_ret;
}

int poplist_destory_cb_free_item_content(void)
{
    int i = 0 ;
    int item_num = 0 ;
    char **p_item_content = NULL ;

    item_num = s_pop_list_param.item_num ;
    p_item_content = s_pop_list_param.item_content ;

    if ( ( item_num > 0) && (s_pop_list_param.item_content != NULL))
    {
        for(i=0;i<item_num;i++)
        {
            if(p_item_content[i] != NULL)
            {
                GxCore_Free(p_item_content[i]);
            }
        }
        GxCore_Free(p_item_content);
        s_pop_list_param.item_content = NULL ;
        return 0 ;
    }
    else
    {
        app_log_error("poplist free err = %d \n",item_num);
    }

    return -1 ;
}

int app_pop_list_end_dialog(void)
{
    if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
    {
        s_select_ret = s_pop_list_param.sel;
        GUI_EndDialog(WND_POP_OPTION_LIST);
        GUI_SetInterface("flush",NULL);
        if(POP_MODE_UNBLOCK == s_pop_list_param.mode)
        {
            if(s_pop_list_param.exit_cb != NULL)
            {
                s_pop_list_param.exit_cb(s_select_ret, s_RetKey);
            }
        }
    }
    s_pop_list_sate = POP_LIST_END;
    return 0;
}
