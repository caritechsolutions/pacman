#include "app_wnd_file_list.h"
#include "app_utility.h"
#include "app_windows.h"

#define LIST_VIEW_FILE_LIST "listview_file_list"
#define SCROLL_FILE_LIST "scroll_file_list"
#define TXT_PATH_INFO "text_file_list_path"
#define IMG_LAST "img_file_list_last"
#define IMG_NEXT "img_file_list_next"
#define IMG_OK "img_file_list_ok"
#define IMG_FILE "MP_ICON_UNKNOW"
#define IMG_DIR "MP_ICON_FILE"
#define IMG_PLAYLIST "MP_ICON_PLAYLIST"

#define POS_X APP_WND_SUBSET_FILE_LIST_POS_X*APP_THEME_XRES/1280
#define POS_Y 130*APP_THEME_YRES/720

typedef enum
{
	FILE_MODE_PARTITION,
	FILE_MODE_FILE,
	FILE_MODE_INVALID
}FileListMode;

typedef struct
{
	char			path[500];
	int			nents;
	GxDirent*	ents;
}DirPara;

typedef struct
{
	bool	last_info;
	bool next_info;
	bool ok_info;
}FileListInfo;

//static DestPathMode s_dest_path_mode = DEST_MODE_FILE;
static WndStatus s_file_list_state = WND_EXEC;
static HotplugPartitionList* partition_list = NULL;
static FileListMode s_file_list_mode = FILE_MODE_PARTITION;
static DirPara s_cur_dir_para = {{0},0,0};
static FileListInfo s_file_list_info = {false};

static FileListParam s_file_list_param;

extern void app_free_dir_ent(GxDirent **ents, int *nents);
extern void top_usb_plug_info(int  state);

static int _file_list_list_get_total(void)
{
#define MAX_PROG_LIST_ITEM 7
	int num = 0;

	if(FILE_MODE_PARTITION == s_file_list_mode)
	{
        if(partition_list && partition_list->partition_num > 0)
            num = partition_list->partition_num;
	}
	else if(FILE_MODE_FILE== s_file_list_mode)
	{
		num = s_cur_dir_para.nents;
	}
	else
	{
		num = 0;
	}

	if(num > MAX_PROG_LIST_ITEM)
	{
		GUI_SetProperty(SCROLL_FILE_LIST, "format", "scroll_show");
	}
	else
	{
		GUI_SetProperty(SCROLL_FILE_LIST, "format", "scroll_hide");
	}

	return num;
}

static status_t app_filelist_getdir(DirPara *dest_dir_para, char *suffix)
{
	if(dest_dir_para == NULL)
		return GXCORE_ERROR;

	app_free_dir_ent(&(dest_dir_para->ents), &(dest_dir_para->nents));
	dest_dir_para->nents = GxCore_GetDir(dest_dir_para->path, &(dest_dir_para->ents), suffix);
	if(dest_dir_para->nents > 0)
	{
		GxCore_SortDir(dest_dir_para->ents, dest_dir_para->nents, NULL);
	}
	else
	{
		app_free_dir_ent(&(dest_dir_para->ents), &(dest_dir_para->nents));
	}
	return GXCORE_SUCCESS;
}

//free the reslut extern if result != NULL
static char* app_filelist_get_parrent_path(char *path)
{
	int len = 0;
	int temp_len = 0;
	char *temp_path = NULL;
	char *ptr = NULL;

	if(path == NULL)
		return NULL;

	len = strlen(path);
	if(len <= 0)
		return NULL;

	temp_path = (char *)GxCore_Malloc(len);
	if(temp_path == NULL)
		return NULL;

	memcpy(temp_path, path, len);

	ptr = temp_path + len;
	temp_len = len;
	while(ptr != temp_path)
	{
		if(*(--ptr) ==  '/')
		{
			temp_len--;
			temp_path[temp_len] = '\0';

			break;
		}
		temp_len--;
		temp_path[temp_len] = '\0';
	}

	if(temp_len == 0)
	{
		GxCore_Free(temp_path);
		temp_path = NULL;
	}

	return temp_path;
}

static FileListMode app_filelist_get_path_mode(char *path)
{
 	FileListMode mode;

	if(path == NULL)
	{
		mode = FILE_MODE_INVALID;
	}
	else
	{
		char *ptr = path;
		int count = 0;

		while(*ptr != '\0')
		{
			if(*ptr == '/')
			{
				count++;
			}
			ptr++;
		}

		if(count < 3)
			mode = FILE_MODE_PARTITION;// /mnt/
		else
			mode = FILE_MODE_FILE; // /mnt/usb_*/
	}
	return mode;
}

static int app_filelist_get_sel(char *path, char *suffix)
{
	int sel = -1;
	FileListMode mode;
	int count = 0;

	if(path == NULL)
		return sel;

	mode = app_filelist_get_path_mode(path);
	if(mode == FILE_MODE_INVALID)
		return sel;

	if(mode == FILE_MODE_PARTITION)
	{
		for(count = 0; count < partition_list->partition_num; count++)
		{
			if(strcmp(path, partition_list->partition[count].partition_entry) == 0)
			{
				sel = count;
				break;
			}
		}
	}
	else
	{
		char *parrent_path = NULL;

		parrent_path = app_filelist_get_parrent_path(path);
		if(parrent_path != NULL)
		{
			DirPara parrent_dir = {{0},0};
			int len = 0;

			len = strlen(parrent_path);
			memcpy(parrent_dir.path, parrent_path, len);

			GxCore_Free(parrent_path);
			parrent_path = NULL;

			app_filelist_getdir(&parrent_dir, suffix);
			strcat(parrent_dir.path, "/");
			for(count = 0; count < parrent_dir.nents; count++)
			{
				int name_len = 0;

				name_len = strlen(parrent_dir.ents[count].fname);
				memcpy(parrent_dir.path + len + 1, parrent_dir.ents[count].fname, name_len);
				parrent_dir.path[len + name_len + 1] = '\0';

				if(strcmp(path, parrent_dir.path) == 0)
				{
					sel = count;
					break;
				}
			}
			app_free_dir_ent(&(parrent_dir.ents), &(parrent_dir.nents));
		}
	}

	return sel;
}

static void app_file_list_set_info(FileListInfo *list_info)
{
	if(list_info->last_info == true)
		GUI_SetProperty(IMG_LAST, "state", "show");
	else
		GUI_SetProperty(IMG_LAST, "state", "hide");

	if(list_info->ok_info== true)
		GUI_SetProperty(IMG_OK, "state", "show");
	else
		GUI_SetProperty(IMG_OK, "state", "hide");

	if(list_info->next_info== true)
		GUI_SetProperty(IMG_NEXT, "state", "show");
	else
		GUI_SetProperty(IMG_NEXT,"state", "hide");

}

static void _file_list_list_change(void)
{
	int sel = 0;

	s_file_list_info.last_info = false;
	s_file_list_info.next_info = false;
	s_file_list_info.ok_info = false;

	if(FILE_MODE_PARTITION == s_file_list_mode)
	{
		if(partition_list->partition_num > 0)
		{
			//if(s_file_list_param.dest_mode == DEST_MODE_DIR)
			{
				s_file_list_info.ok_info= true;
			}
			s_file_list_info.next_info= true;
		}
	}
	else if(FILE_MODE_FILE== s_file_list_mode)
	{
		if(s_cur_dir_para.nents > 0)
		{
			GUI_GetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
			if(s_file_list_param.dest_mode== DEST_MODE_FILE)
			{
				//if(GX_FILE_REGULAR == s_cur_dir_para.ents[sel].ftype)
				{
					s_file_list_info.ok_info= true;
				}
			}
			else if(s_file_list_param.dest_mode == DEST_MODE_DIR)
			{
				if(GX_FILE_DIRECTORY == s_cur_dir_para.ents[sel].ftype)
				{
					s_file_list_info.ok_info= true;
				}
			}

			if(GX_FILE_DIRECTORY == s_cur_dir_para.ents[sel].ftype)
			{
				s_file_list_info.next_info= true;
			}
		}
		s_file_list_info.last_info = true;
	}

	app_file_list_set_info(&s_file_list_info);
}

static void app_file_list_last_dir(int sel)
{
	int set_sel = 0;
	char *info = NULL;

	if(FILE_MODE_FILE == s_file_list_mode)
	{
		set_sel = app_filelist_get_sel(s_cur_dir_para.path, s_file_list_param.suffix);
		if(set_sel < 0)
		{
			set_sel = 0;
			app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
			memset(&s_cur_dir_para, 0, sizeof(DirPara));
			strcpy(s_cur_dir_para.path, partition_list->partition[0].partition_entry);
			s_file_list_mode = FILE_MODE_PARTITION;
		}
		else
		{
			FileListMode mode;
			mode = app_filelist_get_path_mode(s_cur_dir_para.path);
			if(mode == FILE_MODE_INVALID)
			{
				set_sel = 0;
				app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
				memset(&s_cur_dir_para, 0, sizeof(DirPara));
				strcpy(s_cur_dir_para.path, partition_list->partition[0].partition_entry);
				s_file_list_mode = FILE_MODE_PARTITION;
			}
			else if(mode == FILE_MODE_PARTITION && sel >= 0)
			{
				app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
				memset(&s_cur_dir_para, 0, sizeof(DirPara));
				strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
				s_file_list_mode = FILE_MODE_PARTITION;
			}
			else
			{
				char *parrent_path = NULL;
				parrent_path = app_filelist_get_parrent_path(s_cur_dir_para.path);
				if(parrent_path != NULL && strcmp(parrent_path,"/mnt") != 0)
				{
					int len =0;
					len = strlen(parrent_path);
					app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
					memset(&s_cur_dir_para, 0, sizeof(DirPara));
					memcpy(s_cur_dir_para.path, parrent_path, len);
					app_filelist_getdir(&s_cur_dir_para, s_file_list_param.suffix);
					GxCore_Free(parrent_path);
					parrent_path = NULL;
                    if(app_filelist_get_parrent_path(s_cur_dir_para.path)==NULL)
                        s_file_list_mode = FILE_MODE_PARTITION;
                    else
                        s_file_list_mode = FILE_MODE_FILE;
					info = s_cur_dir_para.path;
				}
				else
				{
					set_sel = 0;
					app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
					memset(&s_cur_dir_para, 0, sizeof(DirPara));
					strcpy(s_cur_dir_para.path, partition_list->partition[0].partition_entry);
					s_file_list_mode = FILE_MODE_PARTITION;
				}
			}
		}
	}

	GUI_SetProperty(LIST_VIEW_FILE_LIST, "update_all", NULL);
	GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &set_sel);
	GUI_SetProperty(TXT_PATH_INFO, "string", info);
	_file_list_list_change();
	GUI_SetProperty(WND_FILE_LIST, "update_all", NULL);
}

static void app_file_list_next_dir(int sel)
{
	int set_sel = 0;
	char *info = NULL;

	if(FILE_MODE_PARTITION == s_file_list_mode)
	{
		app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
		memset(&s_cur_dir_para, 0, sizeof(DirPara));

		if((sel >=0)
			&& ( partition_list->partition_num > sel))
		{
			strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
			app_filelist_getdir(&s_cur_dir_para, s_file_list_param.suffix);
			s_file_list_mode = FILE_MODE_FILE;

			GUI_SetProperty(LIST_VIEW_FILE_LIST, "update_all", NULL);
			if(s_cur_dir_para.nents  > 0)
			{
				GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &set_sel);
			}
			else
			{
				set_sel = -1;
				GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &set_sel);
			}
			info = s_cur_dir_para.path;
		}
	}
	else  if(FILE_MODE_FILE == s_file_list_mode)
	{
		if((sel >=0)
			&&(s_cur_dir_para.nents > sel)
			&&(GX_FILE_DIRECTORY == s_cur_dir_para.ents[sel].ftype))
		{
			if(0 == strcasecmp( s_cur_dir_para.ents[sel].fname, ".."))
			{
				app_file_list_last_dir(sel);
				return;
			}
            if(sizeof(s_cur_dir_para.path) < (strlen(s_cur_dir_para.path) + strlen(s_cur_dir_para.ents[sel].fname) + 2))
            {
                PopDlg pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.format = POP_FORMAT_DLG;
				pop.str = STR_ID_ERR_FILE_OPEN;
                pop.mode = POP_MODE_UNBLOCK;
                popdlg_create(&pop);
                return;
            }
            strcat(s_cur_dir_para.path, "/");
			strcat(s_cur_dir_para.path,  s_cur_dir_para.ents[sel].fname);
			app_filelist_getdir(&s_cur_dir_para, s_file_list_param.suffix);
			GUI_SetProperty(LIST_VIEW_FILE_LIST, "update_all", NULL);
			if(s_cur_dir_para.nents  > 0)
			{
				GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &set_sel);
			}
			else
			{
				set_sel = -1;
				GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &set_sel);
			}
		}
		info = s_cur_dir_para.path;
	}
	else
		;

	GUI_SetProperty(TXT_PATH_INFO, "string", info);
	_file_list_list_change();
	GUI_SetProperty(WND_FILE_LIST, "update_all", NULL);
}

WndStatus app_get_file_path_dlg(FileListParam *file_list)
{
	int sel = 0;
	char *info = NULL;

	if((file_list == NULL) || (file_list->dest_path == NULL))
		return WND_CANCLE;

	memcpy(&s_file_list_param, file_list, sizeof(FileListParam));

	app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
	memset(&s_cur_dir_para, 0, sizeof(DirPara));

	partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if((NULL != partition_list) && (0 < partition_list->partition_num))
	{
		sel = app_filelist_get_sel(s_file_list_param.cur_path, s_file_list_param.suffix);
		if(sel < 0)
		{
			sel = 0;
			strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
			s_file_list_mode = FILE_MODE_PARTITION;
		}
		else
		{
			FileListMode mode;
			mode = app_filelist_get_path_mode(s_file_list_param.cur_path);
			if(mode == FILE_MODE_PARTITION)
			{
				strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
				s_file_list_mode = FILE_MODE_PARTITION;
			}
			else if(mode == FILE_MODE_FILE)
			{
				char *parrent_path = NULL;
				parrent_path = app_filelist_get_parrent_path(s_file_list_param.cur_path);
				if(parrent_path != NULL)
				{
					int len =0;
					len = strlen(parrent_path);
					memcpy(s_cur_dir_para.path, parrent_path, len);
					app_filelist_getdir(&s_cur_dir_para, s_file_list_param.suffix);
					GxCore_Free(parrent_path);
					parrent_path = NULL;
					s_file_list_mode = FILE_MODE_FILE;

					info = s_cur_dir_para.path;
				}
				else
				{
					sel = 0;
					memset(&s_cur_dir_para, 0, sizeof(DirPara));
					strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
					s_file_list_mode = FILE_MODE_PARTITION;
				}

			}
			else
				;
		}
	}
	else
	{
        partition_list = NULL;
		sel = -1;
		s_file_list_mode = FILE_MODE_INVALID;
		//info = "warning";
	}

	GUI_CreateDialog(WND_FILE_LIST);

	if((s_file_list_param.pos_x == 0) && (s_file_list_param.pos_y== 0))
	{
		s_file_list_param.pos_x = POS_X;
		s_file_list_param.pos_y = POS_Y;
	}
	GUI_SetProperty(WND_FILE_LIST, "move_window_x", &s_file_list_param.pos_x);
	GUI_SetProperty(WND_FILE_LIST, "move_window_y", &s_file_list_param.pos_y);
	GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
	GUI_SetProperty(TXT_PATH_INFO, "string", info);
	_file_list_list_change();
	s_file_list_state = WND_EXEC;

	return WND_EXEC;
}

void file_list_destroy(void)
{
	GUI_EndDialog(WND_FILE_LIST);
    GUI_SetInterface("flush",NULL);
    app_free_dir_ent(&(s_cur_dir_para.ents), &(s_cur_dir_para.nents));
}

void app_get_file_real_path(char **dest_path)
{
	if ( dest_path != NULL)
	{
		*dest_path = s_cur_dir_para.path;
	}
}

int app_update_xml_m3u_path(char *cur_path, char **dest_path, int (*exit_cb)(WndStatus))
{
#define UPDATE_FILE_SUFFFIX    "xml;XML;m3u;M3U;m3u8;M3U8"
    FileListParam file_para;

    if (check_usb_status() == false)
        return -1;

    memset(&file_para, 0, sizeof(file_para));
    file_para.cur_path = cur_path;
    file_para.dest_path = dest_path;
    file_para.suffix = UPDATE_FILE_SUFFFIX;
    file_para.dest_mode = DEST_MODE_FILE;
    file_para.exit_cb = exit_cb;
    app_get_file_path_dlg(&file_para);
    return 0;
}



static void _file_list_all_destroy(WndStatus ret)
{
	file_list_destroy();
	if (s_file_list_param.exit_cb != NULL )
	{
		s_file_list_param.exit_cb(ret);
	}
}

SIGNAL_HANDLER int app_file_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_file_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_file_list_service(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    GxMessage * msg;
    APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

    event = (GUI_Event *)usrdata;
    msg = (GxMessage*)event->msg.service_msg;
    switch(msg->msg_id)
    {
        case GXMSG_PLAYER_STATUS_REPORT:
        {
            if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
            {
                if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_SUBT))
                {
                    GUI_EndDialog(WIN_MOVIE_SUBT);
                }
                GUI_SendEvent(WIN_MOVIE_VIEW, event);
            }
        }
            break;
        default:
            break;
    }
        return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_file_list_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_file_list_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_file_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _file_list_list_get_total();
}

SIGNAL_HANDLER int app_file_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	int sel = 0;

	GUI_GetProperty(LIST_VIEW_FILE_LIST, "select", &sel);

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
				s_file_list_state = WND_CANCLE;
				_file_list_all_destroy(WND_CANCLE);
				break;

			case STBK_OK:
				if(sel < 0)
					break;

				if(s_file_list_param.dest_mode == DEST_MODE_FILE)
				{
					if(s_file_list_mode == FILE_MODE_FILE)
					{
                        if(NULL == s_cur_dir_para.ents || s_cur_dir_para.nents <= 0)
                            break;

						if(GX_FILE_REGULAR == s_cur_dir_para.ents[sel].ftype)
						{
							strcat(s_cur_dir_para.path, "/");
							strcat(s_cur_dir_para.path,  s_cur_dir_para.ents[sel].fname);
							s_file_list_state = WND_OK;
							_file_list_all_destroy(WND_OK);
						}
						else if(GX_FILE_DIRECTORY == s_cur_dir_para.ents[sel].ftype)
						{
							app_file_list_next_dir(sel);
						}
					}
					else
					{
						app_file_list_next_dir(sel);
					}

				}
				else if(s_file_list_param.dest_mode == DEST_MODE_DIR)
				{
					if(s_file_list_mode == FILE_MODE_PARTITION)
					{
						strcpy(s_cur_dir_para.path, partition_list->partition[sel].partition_entry);
						s_file_list_state = WND_OK;
						_file_list_all_destroy(WND_OK);
					}
					else if(s_file_list_mode == FILE_MODE_FILE)
					{
						if(GX_FILE_DIRECTORY == s_cur_dir_para.ents[sel].ftype)
						{
							strcat(s_cur_dir_para.path, "/");
							strcat(s_cur_dir_para.path,  s_cur_dir_para.ents[sel].fname);
							s_file_list_state = WND_OK;
							_file_list_all_destroy(WND_OK);
						}
					}
				}
				else
					;
				break;

			case STBK_LEFT:
				app_file_list_last_dir(sel);
				break;

			case STBK_RIGHT:
				app_file_list_next_dir(sel);
				break;
			case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
                    if(sel < 0)
                        break;

					if (sel == 0)
					{
						sel = _file_list_list_get_total() - 1;
						GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
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
					GUI_GetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
                    if(sel < 0)
                        break;

					if (_file_list_list_get_total() == sel+1)
					{
						sel = 0;
						GUI_SetProperty(LIST_VIEW_FILE_LIST, "select", &sel);
						ret =  EVENT_TRANSFER_STOP;
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON;
					}
				}
				break;

			default:
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_file_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NAME_LEN 10
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_KEEPON;

	if(0 > item->sel)
		return EVENT_TRANSFER_KEEPON;

	if(FILE_MODE_PARTITION == s_file_list_mode)
	{
		if(item->sel < partition_list->partition_num)
		{
			//col-0: num
			item->x_offset = 0;
			item->image = IMG_DIR;
			item->string = NULL;

			//col-1: channel name
			item = item->next;
			item->x_offset = 0;
			item->image = NULL;
			item->string = partition_list->partition[item->sel].partition_name;
		}
	}
	else if(FILE_MODE_FILE== s_file_list_mode)
	{
		if(item->sel < s_cur_dir_para.nents)
		{
			//col-0: num
			item->x_offset = 0;
			if(GX_FILE_DIRECTORY == s_cur_dir_para.ents[item->sel].ftype)
			{
			    item->image = IMG_DIR;
			}
			else if(GX_FILE_REGULAR == s_cur_dir_para.ents[item->sel].ftype)
			{
				char* file_suffix = NULL;
				file_suffix = file_view_get_file_suffix(s_cur_dir_para.ents[item->sel].fname);
				if(NULL != file_suffix)
				{
					if(0 == strcasecmp(file_suffix, "m3u")
						|| 0 == strcasecmp(file_suffix, "m3u8"))
					{
						item->image = IMG_PLAYLIST;
					}
					else
					{
						item->image = IMG_FILE;
					}
				}
				else
				{
					item->image = IMG_FILE;
				}
			}
			else
			{
			    item->image = NULL;
			}
			item->string = NULL;

			//col-1: channel name
			item = item->next;
			item->x_offset = 0;
			item->image = NULL;
			item->string = s_cur_dir_para.ents[item->sel].fname;
		}
	}
	else
		;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_file_list_list_change(GuiWidget * widget, void *usrdata)
{
	_file_list_list_change();
	return EVENT_TRANSFER_STOP;
}
