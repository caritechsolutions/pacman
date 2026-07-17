#include "app.h"
#include "app_samba.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_windows.h"
#include "pmp_psi.h"
#if PARENTAL_LOCK_SUPPORT
#include"app_parent_lock.h"
#endif

//resourse
#define IMG_DEV_UNSELECT		"MP_ICON_ARROE_UNSELECT"
#define IMG_DEV_SELECT			"MP_ICON_ARROE_SELECT"
#define IMG_DIR					"MP_ICON_FILE"
#define IMG_MOVIC				"MP_ICON_MOVICE"
#define IMG_PIC					"MP_ICON_PIC"
#define IMG_MUSIC				"MP_ICON_MUSIC"
#define IMG_TEXT				"MP_ICON_TXT"
#define IMG_UNKNOW				"MP_ICON_UNKNOW"
#ifdef LINUX_OS
#define IMG_USB					"MP_ICON_USB"
#define IMG_SD					"MP_ICON_SD"
#else
#define IMG_USB					"MP_ICON_FILE"
#define IMG_SD					"MP_ICON_FILE"
#endif
#define IMG_NET					"network_connect"

//widget
#define IMAGE_TITLE				"file_browser_image1"
#define TEXT_TITLE				"file_browser_text_title"
#define TEXT_PATH1				"file_browser_text_path1"
#define TEXT_PATH2				"file_browser_text_path2"
#define IMAGE_USB_SELECT		"file_browser_image_usb"
#define IMAGE_SD_SELECT			"file_browser_image_sd"
#define IMAGE_ROOT_SELECT		"file_browser_image_root"
#define BUTTON_USB				"file_browser_button_usb"
#define BUTTON_SD				"file_browser_button_sd"
#define BUTTON_ROOT				"file_browser_button_root"
#define LISTVIEW_FILE_BROWSER	"file_browser_listview"
#define TEXT_NO_FILE_TIP        "file_browser_no_file_tip_text"
#define BOX_NO_FILE_TIP        "file_browser_no_file_tip_box"
//add in 20120219
#define WIN_FILE_IMG_PATH		"win_file_browser_image_path"
#define BACKGROUND_IMAG			"win_file_browser_image_background"
#define	USB_SETTING_IMAGE		"win_file_browser_image_set"
#define USB_SETTING_TEXT		"win_file_browser_text_setting"
#if MULTIMEDIA_PREVIEW_SUPPORT
#define IMG_FILE_BROWSER_PLAY "img_file_browser_play"
#define IMG_FILE_BROWSER_PAUSE "img_file_browser_pause"
#define IMG_FILE_BROWSER_STOP "img_file_browser_stop"
#define TXT_FILE_BROWSER_SIGNAL "txt_file_browser_signal"
#define IMG_FILE_BROWSER_SIGNAL "img_file_browser_signal"
#define TXT_FILE_BROWSER_NAME "txt_file_browser_file_name"
#define TXT_FILE_BROWSER_SIZE "txt_file_browser_file_size"
#define TXT_FILE_BROWSER_RATIO "txt_file_browser_file_ratio"
#define FILE_BROWSER_TIME_START "txt_file_browser_start_time"
#define FILE_BROWSER_TIME_STOP "txt_file_browser_stop_time"
#define FILE_BROWSER_SLID "file_browser_sliderbar"


#define PLAY_FOCUS_IMG		"s_mpicon_play_focus"
#define PLAY_UNFOCUS_IMG	"s_mpicon_play_unfocus"
#define PAUSE_FOCUS_IMG		"s_mpicon_pause_focus"
#define PAUSE_UNFOCUS_IMG	"s_mpicon_pause_unfocus"
#define PLAY_USB_IMG        "MP_ICON_USB_PLAY"
#define PLAY_FILE_IMG       "MP_ICON_FILE_PLAY"
#define PLAY_ERROR_IMG      "MP_ICON_ERROR_PLAY"
#define PLAY_MOVIE_IMG      "MP_ICON_MOVIE_PLAY"
#define PLAY_MUSIC_IMG      "MP_ICON_MUSIC_PLAY"
#endif

typedef enum
{
	LISTVIEW_MODE_PARTITION,
	LISTVIEW_MODE_FILE
}listview_mode;

typedef enum
{
	DEVICE_USB,
	DEVICE_SD,
	DEVICE_NET,
	DEVICE_COUNT,
}DeviceType_t;

#if MULTIMEDIA_PREVIEW_SUPPORT
typedef struct MiniMediaPlayState
{
    play_movie_ctrol_state state;
    file_view_group group;
}MiniMediaPlayState;

static MiniMediaPlayState s_minimediaplaystate;
static event_list* s_listswitch_timer = NULL;
static event_list* s_playtime_timer = NULL;
static int _file_browser_up_down_timer(void* data);
static int _file_browser_play_time_set(void* data);
static int _file_browser_list_change_start(void);
static void _file_browser_set_video_state(bool video_flag ,bool state_flag, char* image);
extern void play_get_psi_info(char* path,PmpPsi* psi_info);
#endif

static char s_mcenter_dev_name[32] = {0};
static char s_playing_music_dev_name[32] = {0};
static HotplugPartitionList *thiz_partition_list = NULL;
static DeviceType_t DeviceType = DEVICE_USB;
explorer_para* explorer_view = NULL;
HotplugPartitionList* partition_list = NULL;

static int service_status_report(GxMsgProperty_PlayerStatusReport* player_status);
static void _file_browser_update_mcenter_dev_name(uint32_t index);
static void _file_browser_samba_mode_set(listview_mode mode);
#if SAMBA_SUPPORT
static void _file_browser_samba_timer_start(void);
static void _file_browser_samba_timer_stop(void);
#endif
static int _file_browser_init(void);

extern bool music_play_state(void);
extern void music_status_init(void);
extern int text_file_get_open_size(void);
extern  status_t HotplugPartitionDestroy(struct gxfs_hot_device* device);

static int DEVICE_IS_CHOSE(DeviceType_t Type)
{
	if(DeviceType == Type)
		return 1;

	return 0;
}

static int set_path1(char* path)
{
	if(NULL == path)
		GUI_SetProperty(TEXT_PATH1, "string", " ");
	else
		GUI_SetProperty(TEXT_PATH1, "string", path);

	return 0;
}

static int set_path2(char *path)
{
    if(NULL == path)
    {
        GUI_SetProperty(TEXT_PATH2, "format", "static");
        GUI_SetProperty(TEXT_PATH2, "string", " ");
    }
    else if((strlen(path) == 1) && (path[0] == '/'))
    {
        GUI_SetProperty(TEXT_PATH2, "format", "static");
        GUI_SetProperty(TEXT_PATH2, "string", path);
    }
    else
    {
        /*path2: view usb or mmc, cut head, eg /mnt/usb0_1 */
        char* path_cut = NULL;
        int path_depth_cut = 1;
        int slash_num = 0;
        int i;

        if(strstr(s_mcenter_dev_name, "samba") != NULL)
            slash_num = 4;
        else
            slash_num = 3;

        for(i = 1; i < strlen(path); i++)
        {
            if('/' == path[i])
                path_depth_cut++;

            if(slash_num == path_depth_cut)
            {
                if(i + 1 < strlen(path))
                    path_cut = path + i + 1;
                else
                    path_cut = NULL;
                break;
            }
        }

        if(NULL == path_cut)
        {
            GUI_SetProperty(TEXT_PATH2, "format", "static");
            path_cut = " ";
        }
        else
        {
            GUI_SetProperty(TEXT_PATH2, "format", "fix_r2l");
        }
        GUI_SetProperty(TEXT_PATH2, "string", path_cut);
    }

    return 0;
}

listview_mode view_mode_value = LISTVIEW_MODE_PARTITION;
listview_mode view_mode_value_record = LISTVIEW_MODE_PARTITION;
static int check_mode_change(void)
{
	return (view_mode_value_record != view_mode_value)? 1:0;
}

static int fs_setting_control(void)
{
	if(check_mode_change())
	{
#ifdef FILE_EDIT_VALID
#ifdef MEDIA_FILE_EDIT_UNVALID
	return TRUE;
#endif
		switch(view_mode_value)
		{
			case LISTVIEW_MODE_PARTITION:
				GUI_SetProperty(USB_SETTING_IMAGE,"state","hide");//osd_trans_hide
				GUI_SetProperty(USB_SETTING_TEXT,"state","hide");
				break;
			case LISTVIEW_MODE_FILE:
				GUI_SetProperty(USB_SETTING_IMAGE,"state","show");
				GUI_SetProperty(USB_SETTING_TEXT,"state","show");
				break;
			default:
				break;
		}
#endif
		return TRUE;
	}

	return FALSE;
}
static int listview_mode_set(listview_mode mode)
{
	view_mode_value_record = view_mode_value;
	view_mode_value = mode;
	fs_setting_control();
	_file_browser_samba_mode_set(mode);
	return 0;
}

listview_mode listview_mode_get(void)
{
	return view_mode_value;
}

static void _print_partition_info(void)
{
    int i;
    if(NULL == thiz_partition_list)
        return;

    int total = thiz_partition_list->partition_num;

    app_log_info("\033[1;33mUSB PARTITION TOTAL : %d\n\033[0m", total);

    for(i = 0; i < total; i++)
    {
        app_log_info("\033[33m\tName: %s (Mount %s to %s)\n\033[0m",
                thiz_partition_list->partition[i].partition_name,
                thiz_partition_list->partition[i].dev_name,
                thiz_partition_list->partition[i].partition_entry);
    }
    return;
}

static void _file_browser_usb_partition_update(void)
{
    thiz_partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    _print_partition_info();
}

static void _file_browser_usb_partition_clean(void)
{
    thiz_partition_list = NULL;
}

static int _file_browser_get_usb_partition_num(void)
{
    if(thiz_partition_list != NULL)
        return thiz_partition_list->partition_num;

    return 0;
}

static HotplugPartition* _file_browser_get_usb_partition_info(uint32_t index)
{
    int num = _file_browser_get_usb_partition_num();

    if((num > 0) && (index < num))
        return &thiz_partition_list->partition[index];

    return 0;
}

static int device_open(void)
{
	uint32_t index = 0;
	if(DEVICE_IS_CHOSE(DEVICE_USB))
	{
		DeviceType = DEVICE_USB;
		set_path1("USB");
		set_path2(NULL);

		listview_mode_set(LISTVIEW_MODE_PARTITION);
		partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

	}
	else if(DEVICE_IS_CHOSE(DEVICE_SD))
	{
		DeviceType = DEVICE_SD;
		set_path1("SD");
		set_path2(NULL);

		listview_mode_set(LISTVIEW_MODE_PARTITION);
		partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_MMC);
	}
	else if(DEVICE_IS_CHOSE(DEVICE_NET))
	{
		DeviceType = DEVICE_NET;
		set_path1("NET");
		set_path2(NULL);

		listview_mode_set(LISTVIEW_MODE_PARTITION);
		partition_list = NULL;
	}

	GUI_SetFocusWidget(LISTVIEW_FILE_BROWSER);
	GUI_SetProperty(LISTVIEW_FILE_BROWSER, "update_all", NULL);
	GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", &index);

	return 0;
}

#if SAMBA_SUPPORT
static AppSambaListClass thiz_samba_list;
static event_list* s_file_browser_samba_check_timer = NULL;

static void _file_browser_samba_mode_set(listview_mode mode)
{
	switch(mode)
	{
		case LISTVIEW_MODE_PARTITION:
			_file_browser_samba_timer_start();
			break;
		case LISTVIEW_MODE_FILE:
			_file_browser_samba_timer_stop();
			break;
		default:
			break;
	}
}
static void _file_browser_samba_update(void)
{
    AppSambaListClass *p_list = NULL;
    int i;

    memset(&thiz_samba_list, 0, sizeof(AppSambaListClass));
    p_list = app_get_samba_List();
    for(i = 0; i < p_list->samba_num; i++)
    {
        if(app_samba_check_connect(p_list->samba_list[i].samba_id) == true)
            memcpy(&thiz_samba_list.samba_list[thiz_samba_list.samba_num++], &p_list->samba_list[i], sizeof(AppSambaInfoClass));
    }
}

static void _file_browser_samba_clean(void)
{
    memset(&thiz_samba_list, 0, sizeof(AppSambaListClass));
}

static int _file_browser_get_samba_num(void)
{
    return thiz_samba_list.samba_num;
}

static AppSambaInfoClass *_file_browser_get_samba_info(uint32_t index)
{
    int num = _file_browser_get_samba_num();

    if((num > 0) && (index < num))
        return &thiz_samba_list.samba_list[index];

    return NULL;
}

static int _file_browser_samba_check_cb(void* usrdata)
{
    static int num_bak = 0;
    int num = 0;

    _file_browser_samba_update();
    num = _file_browser_get_samba_num();

    if(num != num_bak)
    {
        num_bak = num;
        if(listview_mode_get() == LISTVIEW_MODE_PARTITION)
        {
            _file_browser_init();
            GUI_SetInterface("flush",NULL);
        }
    }

    return 0;
}

static void _file_browser_samba_timer_start(void)
{
    if(0 != reset_timer(s_file_browser_samba_check_timer))
    {
        s_file_browser_samba_check_timer = create_timer(_file_browser_samba_check_cb, 1000, NULL, TIMER_REPEAT);
    }
}

static void _file_browser_samba_timer_stop(void)
{
    if(s_file_browser_samba_check_timer != NULL)
    {
        remove_timer(s_file_browser_samba_check_timer);
        s_file_browser_samba_check_timer = NULL;
    }
}

#else
static void _file_browser_samba_mode_set(listview_mode mode)
{

}
static void _file_browser_samba_update(void)
{
}

static void _file_browser_samba_clean(void)
{
}

static int _file_browser_get_samba_num(void)
{
    return 0;
}

static AppSambaInfoClass *_file_browser_get_samba_info(uint32_t index)
{
    return NULL;
}
#endif

SIGNAL_HANDLER  int file_browser_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;
	GxMsgProperty_PlayerStatusReport* player_status = NULL;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event*)usrdata;
	msg = (GxMessage*)event->msg.service_msg;
	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
			service_status_report(player_status);
			break;

		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

static int _file_browser_find_partition_sel(void)
{
    int i;
    int usb_partition_num = _file_browser_get_usb_partition_num();
    int samba_num = _file_browser_get_samba_num();
    HotplugPartition *partition = NULL;
    AppSambaInfoClass *samba_info = NULL;

    if(usb_partition_num == 0 && samba_num == 0)
        return -1;

    for(i = 0; i < usb_partition_num; i++)
    {
        partition = _file_browser_get_usb_partition_info(i);
        if((partition != NULL) && (strncmp(partition->dev_name, s_mcenter_dev_name, sizeof(s_mcenter_dev_name)) == 0))
            return i;
    }

    for(; i < usb_partition_num + samba_num; i++)
    {
        samba_info = _file_browser_get_samba_info(i - usb_partition_num);
        if((samba_info != NULL) && (strncmp(samba_info->samba_local_path, s_mcenter_dev_name, sizeof(s_mcenter_dev_name)) == 0))
            return i;
    }

    return 0;
}

static int _file_browser_init(void)
{
    int32_t sel = -1;
    file_view_group group = 0;
    int partition_num = 0;
    int samba_num = 0;

	group = file_view_get_record_group();
	if(FILE_VIEW_GROUP_ALL == group)
	{
		GUI_SetProperty(TEXT_TITLE, "string", STR_ID_FILE);
	}
	else if(FILE_VIEW_GROUP_MOVIE == group)
	{
		GUI_SetProperty(TEXT_TITLE, "string", STR_ID_VIDEO);
	}
	else if(FILE_VIEW_GROUP_MUSIC == group)
	{
		GUI_SetProperty(TEXT_TITLE, "string", STR_ID_MUSIC);
	}
	else if(FILE_VIEW_GROUP_PIC == group)
	{
		GUI_SetProperty(TEXT_TITLE, "string", STR_ID_PICTURE);
	}
	else if(FILE_VIEW_GROUP_TEXT == group)
	{
		if (APP_THEME_CLASSIC_BLUE) {
			GUI_SetProperty(TEXT_TITLE, "string", STR_ID_TEXT);
		}
	}

    explorer_closedir(explorer_view);
    explorer_view = NULL;
    listview_mode_set(LISTVIEW_MODE_PARTITION);
    _file_browser_usb_partition_update();
    _file_browser_samba_update();
    partition_num = _file_browser_get_usb_partition_num();
    samba_num =_file_browser_get_samba_num();
    set_path1(NULL);
    set_path2(NULL);
    if((samba_num == 0) && (partition_num == 0))
    {
        GUI_SetProperty(BOX_NO_FILE_TIP, "state", "show");
        GUI_SetProperty(LISTVIEW_FILE_BROWSER, "state", "hide");
        if(LISTVIEW_MODE_FILE == listview_mode_get())
            GUI_SetProperty(TEXT_NO_FILE_TIP, "string", STR_ID_FILE_NOT_EXIST);
        else
            GUI_SetProperty(TEXT_NO_FILE_TIP, "string", STR_ID_MEDIA_DEVICE_NO_EXIST);

    }
    else
    {
        GUI_SetProperty(BOX_NO_FILE_TIP, "state", "hide");
        GUI_SetProperty(LISTVIEW_FILE_BROWSER, "state", "show");
        if (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
            GUI_SetFocusWidget(LISTVIEW_FILE_BROWSER);
        sel = _file_browser_find_partition_sel();

        GUI_SetProperty(LISTVIEW_FILE_BROWSER, "update_all", NULL);
        GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", &sel);
        app_update_pop_dlg(WND_POP_BOOK);

        if(sel < partition_num)
        {
            set_path1("USB");
        }
        else
        {
#if SAMBA_SUPPORT
            if(samba_num > 0)
                set_path1(STR_ID_SAMBA);
#endif
        }
    }
    return 0;
}



void file_brower_book_cancel(void)
{
    explorer_closedir(explorer_view);
	explorer_view = NULL;
	partition_list = NULL;
	device_open();
}

#if MULTIMEDIA_PREVIEW_SUPPORT
static int service_status_report(GxMsgProperty_PlayerStatusReport* player_status)
{
	//char* file_name = NULL;

	APP_CHECK_P(player_status, 1);

	/*play in file_view list, only music support*/
	file_view_group group = FILE_VIEW_GROUP_ALL;
	group = file_view_get_record_group();
	if((FILE_VIEW_GROUP_ALL != group)
		&& (FILE_VIEW_GROUP_MUSIC != group)
        && (FILE_VIEW_GROUP_MOVIE != group)
		&& (0 == strcmp(player_status->player, PMP_PLAYER_AV)))
	{
		play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
		music_status_init();
		return EVENT_TRANSFER_STOP;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_PLAY_END:
			APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
            _file_browser_up_down_timer(NULL);
			break;

        case PLAYER_STATUS_PLAY_RUNNING:
            APP_TIMER_ADD(s_playtime_timer, _file_browser_play_time_set, 200, TIMER_REPEAT);
            APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_RUNNING\n");
            break;

		case PLAYER_STATUS_ERROR:
            _file_browser_set_video_state(false,true,PLAY_ERROR_IMG);
            break;

		default:
			break;
	}

	return 0;
}


static void _file_browser_set_play_pause_stop(bool play_flag, bool pause_flag, bool stop_flag)
{
    char *pImg_focus = NULL;
    char *pImg_unfocus = NULL;
    if (APP_THEME_CLASSIC_BLUE){
        pImg_focus =   "MP_ICON_STOP_YELLOW" ;
        pImg_unfocus = "MP_ICON_STOP" ;
    }
    else{
        pImg_focus =   "s_mpicon_stop_focus" ;
        pImg_unfocus = "s_mpicon_stop_unfocus" ;
    }

    if(play_flag)
        GUI_SetProperty(IMG_FILE_BROWSER_PLAY, "img", PLAY_FOCUS_IMG);
    else
        GUI_SetProperty(IMG_FILE_BROWSER_PLAY, "img", PLAY_UNFOCUS_IMG);

    if(pause_flag)
        GUI_SetProperty(IMG_FILE_BROWSER_PAUSE, "img", PAUSE_FOCUS_IMG);
    else
        GUI_SetProperty(IMG_FILE_BROWSER_PAUSE, "img", PAUSE_UNFOCUS_IMG);

    if(stop_flag)
        GUI_SetProperty(IMG_FILE_BROWSER_STOP, "img", pImg_focus);
    else
        GUI_SetProperty(IMG_FILE_BROWSER_STOP, "img", pImg_unfocus);
}

static void _file_browser_set_video_state(bool video_flag ,bool state_flag, char* image)
{
    if(state_flag)
    {
        GUI_SetProperty(WIN_FILE_BROWSER, "back_ground", "null");
        GUI_SetProperty(TXT_FILE_BROWSER_SIGNAL, "state", "show");
        if(image != NULL)
        {
            GUI_SetProperty(IMG_FILE_BROWSER_SIGNAL, "state", "show");
            GUI_SetProperty(IMG_FILE_BROWSER_SIGNAL, "img", image);
        }
    }
    else
    {
        GUI_SetProperty(TXT_FILE_BROWSER_SIGNAL, "state", "hide");
        GUI_SetProperty(IMG_FILE_BROWSER_SIGNAL, "state", "hide");
    }
    if(video_flag)
    {
       GUI_SetInterface("video_on", NULL);
    }
    else
    {
        GUI_SetInterface("video_off", NULL);
    }
    app_update_pop_dlg(WND_POP_BOOK);
}

static int _filebrowser_lock_play_cb(PopPwdRet *ret, void *usr_data)
{
    uint32_t index = 0;
    char* path = NULL;
    GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
    if(ret->result == PASSWD_OK)
    {
        GxDirent* ent = NULL;
        ent = explorer_view->ents + (index - 1);
        path = explorer_static_path_strcat(explorer_view->path, ent->fname);
        play_av_window_set(TXT_FILE_BROWSER_SIGNAL);
        _file_browser_set_video_state(true,false,NULL);
        play_av_by_url(path, 0);
#if PARENTAL_LOCK_SUPPORT
        PmpPsi psi_info = {0};
        play_get_psi_info(path,&psi_info);
#endif
    }
    else if(ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = _filebrowser_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        _file_browser_list_change_start();
    }
    return 0;
}

static int _file_browser_listview_play(void)
{
    uint32_t index = 0;
    PmpPsi psi_info = {0};
    listview_mode mode = LISTVIEW_MODE_PARTITION;

    if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PAUSE)
    {
        if(s_minimediaplaystate.group == FILE_VIEW_GROUP_MOVIE || s_minimediaplaystate.group == FILE_VIEW_GROUP_MUSIC)
        {
            _file_browser_set_play_pause_stop(false,true,false);
            play_av_by_resume();
            s_minimediaplaystate.state = PLAY_MOVIE_CTROL_PLAY;
            return 0;
        }
    }
    if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PLAY)
    {
        if(s_minimediaplaystate.group == FILE_VIEW_GROUP_MOVIE || s_minimediaplaystate.group == FILE_VIEW_GROUP_MUSIC)
            return 0;
    }
    GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
    mode = listview_mode_get();
    if(LISTVIEW_MODE_PARTITION == mode)
    {
        return -1;
    }
    else  if(LISTVIEW_MODE_FILE == mode)
    {
        if(NULL == explorer_view)
            return -1;

        if(index == 0)
            return -1;

        if(index > explorer_view->nents)
            return -1;

        GxDirent* ent = NULL;
        ent = explorer_view->ents + (index - 1);
        if(NULL == ent)
            return -1;

        if(GX_FILE_DIRECTORY == ent->ftype)
        {
            return -1;
        }
        else if(GX_FILE_REGULAR == ent->ftype)
        {
           file_view_group group = FILE_VIEW_GROUP_ALL;
            group = file_view_get_file_group(ent->fname);
            char *path = NULL;
            path = explorer_static_path_strcat(explorer_view->path, ent->fname);
            if(FILE_VIEW_GROUP_MUSIC == group || FILE_VIEW_GROUP_MOVIE == group)
            {
                _file_browser_set_play_pause_stop(false,true,false);
                if(FILE_VIEW_GROUP_MOVIE == group)
                {
                    _file_browser_set_video_state(true,false,NULL);
                    play_get_psi_info(path,&psi_info);
                    if(psi_info.private_info.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
                    {
                        PasswdDlg passwd_dlg;
                        play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
                        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
                        passwd_dlg.passwd_exit_cb = _filebrowser_lock_play_cb;
                        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
                        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
                        passwd_dlg_create(&passwd_dlg);
                        s_minimediaplaystate.group = group;
                        s_minimediaplaystate.state = PLAY_MOVIE_CTROL_PLAY;
                        return 0;
                    }
                }
                play_av_window_set(TXT_FILE_BROWSER_SIGNAL);
                play_av_by_url(path, 0);
#if PARENTAL_LOCK_SUPPORT
                if(FILE_VIEW_GROUP_MOVIE == group)
                {
                    app_pvr_file_parental_rating_start(1,psi_info.ts_id,psi_info.service_id);
                }
#endif
                s_minimediaplaystate.state = PLAY_MOVIE_CTROL_PLAY;
            }
       }
    }
    return 0;
}

static status_t _file_browser_listview_stop(bool exit_flag)
{
    int cur_sliderbar = 0;
    APP_TIMER_REMOVE(s_playtime_timer);
    if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PLAY || s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PAUSE)
    {
        if(exit_flag == false)
        {
            GUI_SetProperty(FILE_BROWSER_SLID, "value", &cur_sliderbar);
            GUI_SetProperty(FILE_BROWSER_TIME_START, "string", " ");
            _file_browser_set_play_pause_stop(true,false,false);
        }
        else
        {
            GUI_SetProperty(FILE_BROWSER_SLID, "value", &cur_sliderbar);
            GUI_SetProperty(FILE_BROWSER_TIME_START, "string", "00:00:00");
            _file_browser_set_play_pause_stop(false,false,false);
        }
#if PARENTAL_LOCK_SUPPORT
        if(s_minimediaplaystate.group == FILE_VIEW_GROUP_MOVIE)
        {
            app_pvr_file_parental_rating_stop();
        }
#endif
        play_av_by_stop();
        s_minimediaplaystate.state = PLAY_MOVIE_CTROL_STOP;
    }
    return  GXCORE_SUCCESS;
}

static int _file_browser_list_change_start(void)
{
     APP_TIMER_ADD(s_listswitch_timer, _file_browser_up_down_timer, 300, TIMER_REPEAT);
     return 0;
}

static int _file_browser_list_change_stop(void)
{
     APP_TIMER_REMOVE(s_listswitch_timer);
     return 0;
}

static status_t _file_browser_listview_pause(void)
{
    if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PLAY)
    {
         _file_browser_set_play_pause_stop(true,false,false);
         play_av_by_pause();
        s_minimediaplaystate.state = PLAY_MOVIE_CTROL_PAUSE;
    }
    else if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_PAUSE)
    {
        if(s_minimediaplaystate.group == FILE_VIEW_GROUP_MOVIE || s_minimediaplaystate.group == FILE_VIEW_GROUP_MUSIC)
        {
            _file_browser_set_play_pause_stop(false,true,false);
            play_av_by_resume();
            s_minimediaplaystate.state = PLAY_MOVIE_CTROL_PLAY;
        }
    }
    return GXCORE_SUCCESS;
}

#if PARENTAL_LOCK_SUPPORT
int app_filebrowser_parent_lock_play(void)
{
    PasswdDlg passwd_dlg;
    play_av_by_stop();
    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.str = STR_ID_PARENT_GUID;
    passwd_dlg.passwd_exit_cb = _filebrowser_lock_play_cb;
    passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
    passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
    passwd_dlg_create(&passwd_dlg);
    return 0;
}
#endif

static int _file_browser_up_down_timer(void* data)
{
    PlayerProgInfo *player_info = app_player_prog_info_get();
    char *path = NULL;
    uint32_t index = 0;
    char str_size[20] = {0};
    _file_browser_list_change_stop();
    _file_browser_listview_stop(false);
    listview_mode mode = LISTVIEW_MODE_PARTITION;
    GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
    mode = listview_mode_get();
    if(LISTVIEW_MODE_PARTITION == mode)
    {
        HotplugPartition *partition = _file_browser_get_usb_partition_info(index);
        if(partition)
        {
            GUI_SetProperty(TXT_FILE_BROWSER_NAME, "string", partition->partition_name);
            _file_browser_set_video_state(false,true,PLAY_USB_IMG);
            GUI_SetProperty(TXT_FILE_BROWSER_SIZE, "string", " ");
            GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_START, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", " ");
            GUI_SetProperty(FILE_BROWSER_SLID, "state", "hide");
        }
        else
        {
            GUI_SetProperty(TXT_FILE_BROWSER_NAME,"string", " ");
            GUI_SetProperty(TXT_FILE_BROWSER_SIZE, "string", " ");
            GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_START, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", " ");
            GUI_SetProperty(FILE_BROWSER_SLID, "state", "hide");
            _file_browser_set_video_state(false,true,PLAY_ERROR_IMG);
        }
        s_minimediaplaystate.group = FILE_VIEW_GROUP_ALL;
        s_minimediaplaystate.state = PLAY_MOVIE_CTROL_STOP;
        return -1;
    }
    else  if(LISTVIEW_MODE_FILE == mode)
    {
        if(NULL == explorer_view)
            return -1;

        if(index == 0)
            return -1;

        if(index > explorer_view->nents)
            return -1;

        GxDirent* ent = NULL;
        ent = explorer_view->ents + (index - 1);
        if(NULL == ent)
            return -1;

        _file_browser_set_play_pause_stop(false,false,false);
        GUI_SetProperty(TXT_FILE_BROWSER_NAME, "string", ent->fname);
        if(GX_FILE_DIRECTORY == ent->ftype)
        {
            GUI_SetProperty(TXT_FILE_BROWSER_SIZE, "string", " ");
            GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_START, "string", " ");
            GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", " ");
            GUI_SetProperty(FILE_BROWSER_SLID, "state", "hide");
            s_minimediaplaystate.group = FILE_VIEW_GROUP_ALL;
            s_minimediaplaystate.state = PLAY_MOVIE_CTROL_STOP;
            _file_browser_set_video_state(false,true,PLAY_FILE_IMG);
            return -1;
        }
        else if(GX_FILE_REGULAR == ent->ftype)
        {
            memset(str_size,0,sizeof(str_size));
            play_av_display_size_str(ent->fsize,str_size);
            GUI_SetProperty(TXT_FILE_BROWSER_SIZE, "string", str_size);
            file_view_group group = FILE_VIEW_GROUP_ALL;
            group = file_view_get_file_group(ent->fname);
            path = explorer_static_path_strcat(explorer_view->path, ent->fname);
            if(FILE_VIEW_GROUP_MUSIC == group || FILE_VIEW_GROUP_MOVIE == group)
            {
                GUI_SetProperty(FILE_BROWSER_TIME_START, "string", "00:00:00");
                if(GxPlayer_MediaGetProgInfoByUrl(path, player_info) == 0)
                {
                    app_time_ms_to_edit_str(player_info->duration, str_size, 20);
                    GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", str_size);
                    if(player_info->video.width != 0 && player_info->video.height != 0)
                    {
                        memset(str_size,0,sizeof(str_size));
                        sprintf(str_size, "%dx%02d", player_info->video.width,player_info->video.height);
                        GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", str_size);
                    }
                    else
                    {
                        GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
                    }
                }
                else
                {
                    GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
                    GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", "00:00:00");
                }
                GUI_SetProperty(FILE_BROWSER_SLID, "state", "show");
                _file_browser_set_play_pause_stop(true,false,false);//2022
                if(FILE_VIEW_GROUP_MUSIC == group)
                {
                    _file_browser_set_video_state(false,true,PLAY_MUSIC_IMG);
                }
                else
                    _file_browser_set_video_state(false,true,PLAY_MOVIE_IMG);
            }
            else
            {
                GUI_SetProperty(TXT_FILE_BROWSER_RATIO, "string", " ");
                GUI_SetProperty(FILE_BROWSER_TIME_START, "string", " ");
                GUI_SetProperty(FILE_BROWSER_TIME_STOP, "string", " ");
                GUI_SetProperty(FILE_BROWSER_SLID, "state", "hide");
                if(FILE_VIEW_GROUP_PIC == group)
                {
                   if(GDI_PlayImage(path,-1,-1) != 0)
                    {
                        _file_browser_set_video_state(false,true,PLAY_ERROR_IMG);
                    }
                   play_pic_window_set(TXT_FILE_BROWSER_SIGNAL);
                   _file_browser_set_video_state(false,false,NULL);
                }
                else
                {
                    _file_browser_set_video_state(false,true,"MP_ICON_FILE_PLAY");
                }
            }
            s_minimediaplaystate.group = group;
            s_minimediaplaystate.state = PLAY_MOVIE_CTROL_STOP;
        }
    }
    return 0;
}

static int _file_browser_play_time_set(void* data)
{
    uint64_t cur_time_ms = 0;
    uint64_t total_time_ms = 0;
    int cur_sliderbar = 0;
    char buf_tmp[20] = {0};
    status_t ret = GXCORE_ERROR;
    PlayTimeInfo  tinfo = {0};

    ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);
    if(GXCORE_SUCCESS == ret)
    {
        cur_time_ms = tinfo.current - tinfo.seek_min;
        total_time_ms = tinfo.totle -  tinfo.seek_min;
        if(cur_time_ms > total_time_ms)
        {
            cur_sliderbar = 100;
        }
        else
        {
             cur_sliderbar = (cur_time_ms * 100)/ total_time_ms;
        }
        GUI_SetProperty(FILE_BROWSER_SLID, "value", &cur_sliderbar);
        app_time_ms_to_edit_str(cur_time_ms, buf_tmp, 20);
        GUI_SetProperty(FILE_BROWSER_TIME_START, "string", buf_tmp);
    }
    return 0;
}

static void  _file_browser_quit_exit(int key)
{
    _file_browser_list_change_stop();
    _file_browser_listview_stop(true);
    if (APP_THEME_CLASSIC_PURPLE || APP_THEME_GRID_PURPLE){
        app_unset_window_back_ground(WND_FULL_SCREEN, true, false);
    }else{
        app_set_window_back_ground(WND_MAIN_MENU, NULL, true);
    }
    if(key == APPK_OK && s_minimediaplaystate.group == FILE_VIEW_GROUP_MOVIE)
    {
        play_av_window_set_full_screen();
    }
    else if(key == APPK_BACK)
    {
        play_pic_window_set_full_screen();
        play_av_window_set_full_screen();
    }
}

static int app_file_browser_init(void)
{
    _file_browser_init();
    _file_browser_list_change_start();
    s_minimediaplaystate.state = PLAY_MOVIE_CTROL_STOP;
    s_minimediaplaystate.group = FILE_VIEW_GROUP_ALL;
    return 0;
}

static int file_browser_list_change_restart(void)
{
    if(s_minimediaplaystate.state == PLAY_MOVIE_CTROL_STOP)
    {
        _file_browser_list_change_start();
    }
	return EVENT_TRANSFER_STOP;
}
#else

static int service_status_report(GxMsgProperty_PlayerStatusReport* player_status)
{
	//char* file_name = NULL;

	APP_CHECK_P(player_status, 1);

	/*play in file_view list, only music support*/
	file_view_group group = FILE_VIEW_GROUP_ALL;
	group = file_view_get_record_group();
	if((FILE_VIEW_GROUP_ALL != group)
		&& (FILE_VIEW_GROUP_MUSIC != group)
		&& (0 == strcmp(player_status->player, PMP_PLAYER_AV)))
	{
		play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
		music_status_init();
		return EVENT_TRANSFER_STOP;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_PLAY_END:
			APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
			//view pic, player music in background
			if((0 == strcmp(player_status->player, PMP_PLAYER_AV))
				&& (music_play_state() == true))
			{
				//sequence
				pmpset_music_play_sequence sequence = 0;
				sequence = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
				if(PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE == sequence)
				{
					APP_Printf("play only once\n");
					play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
					music_status_init();
					break;
				}
				else if(PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ONE == sequence)
				{
					APP_Printf("play repeat one\n");
					play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
					break;
				}
				else if(PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE == sequence)
				{
					play_list* list = NULL;
					list = play_list_get(PLAY_LIST_TYPE_MUSIC);
					 if(list->play_no >= list->nents -1)
					{
						play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
						music_status_init();
					}
					else
					{
						play_music_ctrol(PLAY_MUSIC_CTROL_NEXT);
					}

					break;
				}
				else
				{
					play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
					music_status_init();
					break;
				}
			}
			break;

        case PLAYER_STATUS_PLAY_RUNNING:
            APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_RUNNING\n");
            break;

		case PLAYER_STATUS_ERROR:
			if((0 == strcmp(player_status->player, PMP_PLAYER_AV))
				&& (music_play_state() == true))
            {
                pmpset_music_play_sequence sequence = 0;
                sequence = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
                if(PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE == sequence)
                {
                    play_list* list = NULL;
                    list = play_list_get(PLAY_LIST_TYPE_MUSIC);
                    if(list->play_no >= list->nents -1)
                    {
                        play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
                        music_status_init();
                    }
                    else
                    {
                        play_music_ctrol(PLAY_MUSIC_CTROL_NEXT);
                    }
                }
                else
                {
                    play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
                }
            }
			break;

		default:
			break;
	}

	return 0;
}

static int _file_browser_list_change_start(void)
{
    return 0;
}

static int _file_browser_list_change_stop(void)
{
    return 0;
}

static int file_browser_list_change_restart(void)
{
    return 0;
}

static void  _file_browser_quit_exit(int key)
{
    _file_browser_list_change_stop();
    return;
}

static int app_file_browser_init(void)
{
    _file_browser_init();
    return 0;
}


#endif

SIGNAL_HANDLER int file_browser_init(const char* widgetname, void *usrdata)
{
    app_file_browser_init();
    return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int file_browser_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    if(NULL == usrdata)
        return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->key.sym)
    {
        case APPK_BACK:
        case APPK_MENU:
            event->key.sym = APPK_BACK;
            return EVENT_TRANSFER_KEEPON;
        case APPK_UP:
        case APPK_DOWN:
            break;

        case STBK_PAGE_UP:
        case STBK_PAGE_DOWN:
            {
                return  EVENT_TRANSFER_STOP;
            }
            break;

        case VK_BOOK_TRIGGER:
            _file_browser_quit_exit(APPK_BACK);
            GUI_EndDialog(WIN_FILE_BROWSER);
            GUI_SendEvent(GUI_GetFocusWindow(), event);
            return  EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }
    return EVENT_TRANSFER_KEEPON;
}

static status_t _file_browser_stop_player(void)
{
    /*stop all player av*/
    play_av_by_stop();
    /*stop all player pic*/
    GxMessage* new_msg_pic = NULL;
    new_msg_pic = GxBus_MessageNew(GXMSG_PLAYER_STOP);
    APP_CHECK_P(new_msg_pic, GXCORE_ERROR);

    GxMsgProperty_PlayerStop *stop_pic;
    stop_pic = GxBus_GetMsgPropertyPtr(new_msg_pic, GxMsgProperty_PlayerStop);
    APP_CHECK_P(stop_pic, GXCORE_ERROR);
    stop_pic->player = PMP_PLAYER_PIC;

    GxBus_MessageSendWait(new_msg_pic);
    GxBus_MessageFree(new_msg_pic);

    if(music_play_state() == TRUE)
    {
        memset(s_playing_music_dev_name, 0, sizeof(s_playing_music_dev_name));
        clean_music_file_no_bak();
        music_status_init();
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int file_browser_destroy(const char* widgetname, void *usrdata)
{
   _file_browser_stop_player();

    explorer_closedir(explorer_view);
    explorer_view = NULL;

    _file_browser_usb_partition_clean();
#if SAMBA_SUPPORT
    _file_browser_samba_timer_stop();
#endif
    _file_browser_samba_clean();

    memset(s_mcenter_dev_name, 0, sizeof(s_mcenter_dev_name));

    if( APP_THEME_CLASSIC_DARK ){
    pmpset_exit();
    int init_value = 0;
    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    }

    return 0;
}

static int _file_browser_listview_get_count(void)
{
	listview_mode mode = LISTVIEW_MODE_PARTITION;
    int item_num = 0;

	mode = listview_mode_get();
	if(LISTVIEW_MODE_PARTITION == mode)
	{
        item_num = _file_browser_get_usb_partition_num();
        item_num += _file_browser_get_samba_num();
        return item_num;
	}
	else if(LISTVIEW_MODE_FILE == mode)
	{
		if(NULL == explorer_view)
			return 0;

		return explorer_view->nents+1;
	}

	return 0;
}

SIGNAL_HANDLER int file_browser_listview_get_count(const char* widgetname, void *usrdata)
{
    return _file_browser_listview_get_count();
}

static void _file_browser_partition_view_get_date(ListItemPara *item)
{
    int usb_partition_num = 0;
    int samba_num = 0;
    uint32_t index = 0;

    if(item == NULL)
        return;

    usb_partition_num = _file_browser_get_usb_partition_num();
    samba_num = _file_browser_get_samba_num();

    index = (uint32_t)item->sel;
    if(index >= usb_partition_num + samba_num)
    {
        item->image = NULL;
        item = item->next;
        item->string = NULL;
        return;
    }

    if(index < usb_partition_num)
    {
        //usb storge
        HotplugPartition *partition = _file_browser_get_usb_partition_info(index);
        if(NULL != partition)
        {
            /*col-0: img*/
            item->x_offset = 0;
            item->string = NULL;
            item->image = IMG_USB;

            /*col-1: partition name*/
            item = item->next;
            if(NULL != item)
            {
                item->x_offset = 0;
                item->image = NULL;
                item->string = partition->partition_name;
            }
        }
    }
    else
    {
        AppSambaInfoClass *samba_info = _file_browser_get_samba_info(index - usb_partition_num);
        if(samba_info != NULL)
        {
            /*col-0: img*/
            item->x_offset = 0;
            item->string = NULL;
            item->image = IMG_NET;

            /*col-1: partition name*/
            item = item->next;
            if(NULL != item)
            {
                item->x_offset = 0;
                item->image = NULL;
                item->string = samba_info->samba_svr_path;
            }
        }
    }
}

SIGNAL_HANDLER int file_browser_listview_get_data(const char* widgetname, void *usrdata)
{
    ListItemPara* item = NULL;
    uint32_t index = 0;
    listview_mode mode = LISTVIEW_MODE_PARTITION;
    char* img_pic = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item) return GXCORE_ERROR;
    index = (uint32_t)item->sel;

    mode = listview_mode_get();

    /*listview view mode: PARTITION*/
    if(LISTVIEW_MODE_PARTITION == mode)
    {
        _file_browser_partition_view_get_date(item);
    }

    /*listview view mode: FILE*/
    else if(LISTVIEW_MODE_FILE == mode)
    {
        if(NULL == explorer_view)
            return GXCORE_ERROR;

        if(index == 0) //last dir
        {
            /*col-0: img*/
            item->x_offset = 0;
            item->string = NULL;
            item->image = NULL;

            /*col-1: file name*/
            item = item->next;
            if(NULL == item) return GXCORE_ERROR;
            item->x_offset = 0;
            item->image = NULL;
            item->string = "/..";

            return GXCORE_SUCCESS;
        }


        if(index > explorer_view->nents)
            return GXCORE_ERROR;

        GxDirent* ent = NULL;
        ent = explorer_view->ents + (index - 1);
        if(NULL == ent)
            return GXCORE_ERROR;

        if(GX_FILE_DIRECTORY == ent->ftype)
        {
            img_pic = IMG_DIR;
        }
        else if(GX_FILE_REGULAR == ent->ftype)
        {
            file_view_group group = FILE_VIEW_GROUP_ALL;
            group = file_view_get_file_group(ent->fname);

            if(FILE_VIEW_GROUP_PIC == group)
            {
                img_pic = IMG_PIC;
            }
            else if(FILE_VIEW_GROUP_MUSIC == group)
            {
                img_pic = IMG_MUSIC;
            }
            else if(FILE_VIEW_GROUP_MOVIE == group)
            {
                img_pic = IMG_MOVIC;
            }
            else if((FILE_VIEW_GROUP_TEXT== group) && APP_THEME_CLASSIC_BLUE)
            {
                img_pic = IMG_TEXT;
            }
            else
            {
                char* file_suffix = NULL;

                file_suffix = file_view_get_file_suffix(ent->fname);
                if(NULL != file_suffix)
                {
                    img_pic = IMG_UNKNOW;
                }
                else
                {
                    img_pic = IMG_UNKNOW;
                }
            }
        }
        /*col-0: img*/
        item->x_offset = 0;
        item->string = NULL;
        item->image = img_pic;

        /*col-1: file name*/
        item = item->next;
        if(NULL == item) return GXCORE_ERROR;
        item->x_offset = 0;
        item->image = NULL;
        item->string = ent->fname;
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int file_browser_listview_change(const char* widgetname, void *usrdata)
{
    listview_mode mode = LISTVIEW_MODE_PARTITION;
    int usb_partition_num = _file_browser_get_usb_partition_num();
    int samba_num = _file_browser_get_samba_num();
    uint32_t index = 0;

    mode = listview_mode_get();

    /*from partition to button*/
    if(LISTVIEW_MODE_PARTITION == mode)
    {
        GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
        if(index >= usb_partition_num + samba_num)
            return EVENT_TRANSFER_STOP;

        _file_browser_update_mcenter_dev_name(index);

        if(index < usb_partition_num)
        {
            set_path1("USB");
        }
        else
        {
#if SAMBA_SUPPORT
            set_path1(STR_ID_SAMBA);
#endif
        }
    }

    return EVENT_TRANSFER_STOP;
}

static int file_browser_listview_backkey(void)
{
    listview_mode mode = LISTVIEW_MODE_PARTITION;
    int listview_sel = 0;

    mode = listview_mode_get();

    /*from partition to button*/
    if(LISTVIEW_MODE_PARTITION == mode)
    {
        _file_browser_quit_exit(APPK_BACK);
        return EVENT_TRANSFER_KEEPON;
    }
    else
    {
        _file_browser_list_change_start();
        if(explorer_view != NULL)
        {
            if(0 == explorer_view->path_depth)
            {
                //device root
                _file_browser_init();
            }
            else
            {
                set_path2(STR_ID_WAITING);
                GUI_SetInterface("flush",NULL);

                explorer_view = explorer_backdir(explorer_view);

                listview_sel = explorer_view->chdir_sel[explorer_view->path_depth];
                GUI_SetProperty(LISTVIEW_FILE_BROWSER, "update_all", NULL);
                GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", (void*)&listview_sel);
                set_path2(explorer_view->path);
            }
        }
    }

    return EVENT_TRANSFER_STOP;
}

static int _file_browser_get_deviceinfo(uint32_t index, char **dpath, char **dname, char **pname)
{
    int usb_partition_num = _file_browser_get_usb_partition_num();
    int samba_num = _file_browser_get_samba_num();

    if(index >= usb_partition_num + samba_num)
        return -1;

    if(index < usb_partition_num)
    {
        //usb device
        HotplugPartition *partition = _file_browser_get_usb_partition_info(index);
        if(partition == NULL)
            return -1;
        *dpath = partition->partition_entry;
        *pname = partition->partition_name;
        *dname = partition->dev_name;
    }
    else
    {
        //samba device
        AppSambaInfoClass *samba_info = _file_browser_get_samba_info(index - usb_partition_num);
        if(samba_info == NULL)
            return -1;
        *dpath = samba_info->samba_local_path;
        *pname = samba_info->samba_svr_path;
        *dname = samba_info->samba_local_path;
    }
    return 0;
}

static void _file_browser_update_mcenter_dev_name(uint32_t index)
{
    char *device_path = NULL;
    char *device_name = NULL;
    char *partition_name = NULL;

    if(0 != _file_browser_get_deviceinfo(index, &device_path, &device_name, &partition_name))
        return;

    memset(s_mcenter_dev_name, 0, sizeof(s_mcenter_dev_name));
    strlcpy(s_mcenter_dev_name, device_name, sizeof(s_mcenter_dev_name));
    return;
}

static void _file_browser_partition_okkey(uint32_t index)
{
    int file_num = 0;
    int listview_sel = 0;
    char *device_path = NULL;
    char *device_name = NULL;
    char *partition_name = NULL;

    if(0 != _file_browser_get_deviceinfo(index, &device_path, &device_name, &partition_name))
        return;

    if((strlen(s_mcenter_dev_name) > 0)
        && (strncmp(s_mcenter_dev_name, device_name, sizeof(s_mcenter_dev_name)) != 0))
    {
        _file_browser_stop_player();
    }

    memset(s_mcenter_dev_name, 0, sizeof(s_mcenter_dev_name));
    strlcpy(s_mcenter_dev_name, device_name, sizeof(s_mcenter_dev_name));

    listview_mode_set(LISTVIEW_MODE_FILE);

    set_path2(STR_ID_WAITING);

    char* group_suffixs = NULL;
    explorer_closedir(explorer_view);
    group_suffixs = file_view_get_group_suffixs(file_view_get_record_group());
    explorer_view = explorer_opendir(device_path, group_suffixs);
    GUI_SetProperty(LISTVIEW_FILE_BROWSER, "update_all", NULL);
    GUI_GetProperty(LISTVIEW_FILE_BROWSER, "total_num", &file_num);
    if(file_num > 1)
        listview_sel = 1;
    GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", (void*)&listview_sel);

    set_path1(partition_name);
    set_path2(NULL); //root
}

static int file_browser_listview_okkey(void)
{
    uint32_t index = 0;
    int listview_sel = 0;
    int file_num = 0;
    listview_mode mode = LISTVIEW_MODE_PARTITION;

    GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);

    mode = listview_mode_get();

    if(LISTVIEW_MODE_PARTITION == mode)
    {
        _file_browser_partition_okkey(index);
        _file_browser_list_change_start();
    }

    /*listview view mode: FILE*/
    else  if(LISTVIEW_MODE_FILE == mode)
    {
        if(NULL == explorer_view)
            return GXCORE_ERROR;

        if(index == 0)
        {
            file_browser_listview_backkey();
            return GXCORE_SUCCESS;
        }

        if(index > explorer_view->nents)
            return GXCORE_ERROR;

        GxDirent* ent = NULL;
        ent = explorer_view->ents + (index - 1);
        if(NULL == ent)
            return GXCORE_ERROR;

        /*enter dir*/
        if(GX_FILE_DIRECTORY == ent->ftype)
        {
            set_path2(STR_ID_WAITING);
            GUI_SetInterface("flush",NULL);

            explorer_para* explorer_old = explorer_view;
            explorer_view = explorer_chdir(explorer_view, index);
            if(NULL == explorer_view)
            {

                explorer_view = explorer_old;
                set_path2(explorer_view->path);
                PopDlg pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.format = POP_FORMAT_DLG;
                pop.str = "Can't open the file!";
                pop.mode = POP_MODE_UNBLOCK;
                popdlg_create(&pop);
                return GXCORE_ERROR;
            }
            GUI_SetProperty(LISTVIEW_FILE_BROWSER, "update_all", NULL);
            GUI_GetProperty(LISTVIEW_FILE_BROWSER, "total_num", &file_num);
            if(file_num > 1)
                listview_sel = 1;
            GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", (void*)&listview_sel);
            set_path2(explorer_view->path);
            _file_browser_list_change_start();
        }

        /*start reference play*/
        else if(GX_FILE_REGULAR == ent->ftype)
        {
            file_view_group group = FILE_VIEW_GROUP_ALL;
            group = file_view_get_file_group(ent->fname);

            if(FILE_VIEW_GROUP_PIC == group)
            {
                play_list_init(PLAY_LIST_TYPE_PIC, explorer_view, index-1);
                GUI_CreateDialog(WIN_PIC_VIEW);
#if MULTIMEDIA_PREVIEW_SUPPORT
                play_pic_window_set_full_screen();
#endif
            }
            else if(FILE_VIEW_GROUP_MUSIC == group)
            {
                memset(s_playing_music_dev_name, 0, sizeof(s_playing_music_dev_name));
                memcpy(s_playing_music_dev_name, s_mcenter_dev_name, sizeof(s_mcenter_dev_name));
                play_list_init(PLAY_LIST_TYPE_MUSIC, explorer_view, index-1);
                GUI_CreateDialog(WIN_MUSIC_VIEW);
            }
            else if(FILE_VIEW_GROUP_MOVIE == group)
            {
                play_list_init(PLAY_LIST_TYPE_MOVIE, explorer_view, index-1);
                GUI_CreateDialog(WIN_MOVIE_VIEW);
            }
#if THEME_CLASSIC_BLUE  //这个只能当宏定义
            else if(FILE_VIEW_GROUP_TEXT== group)
            {

                play_list_init(PLAY_LIST_TYPE_TEXT, explorer_view, index-1);
                {
                #define MAX_TEXT_FILE_SIZE_BIT    (512*1024)
                    int len = 0;
                    len = text_file_get_open_size();
                    if(len > MAX_TEXT_FILE_SIZE_BIT)
                    {
                        return GXCORE_SUCCESS;
                    }
                }
                GUI_CreateDialog(WIN_TEXT_VIEW);
            }
#endif
            else
            {
                PopDlg pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.format = POP_FORMAT_DLG;
                pop.str = STR_ID_FILE_NOT_SUPPORT;
                pop.mode = POP_MODE_UNBLOCK;
                popdlg_create(&pop);
            }
        }
    }

    return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int file_browser_listview_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
    {
        case APPK_BACK:
        case APPK_MENU:
            return file_browser_listview_backkey();
            /*page down*/
        case APPK_PREVIOUS:
            event->key.sym = GUIK_PAGE_UP;
            return EVENT_TRANSFER_KEEPON;
            /*page up*/
        case APPK_NEXT:
            event->key.sym = GUIK_PAGE_DOWN;
            return EVENT_TRANSFER_KEEPON;
        case APPK_OK:
            _file_browser_quit_exit(APPK_OK);
            GUI_SetProperty(GUI_GetFocusWidget(), "rolling_stop", NULL);
            GxCore_ThreadDelay(60);//20121031 for Mantis Bug:864
            file_browser_listview_okkey();
            return EVENT_TRANSFER_STOP;

        case APPK_SET:
            {
                return EVENT_TRANSFER_STOP;
            }
        case APPK_PAGE_UP:
            {
                _file_browser_list_change_start();
                uint32_t sel;
                GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &sel);
                if (sel == 0)
                {
                    sel = _file_browser_listview_get_count() - 1;
                    GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", &sel);
                    return  EVENT_TRANSFER_STOP;
                }
                else
                {
                    return  EVENT_TRANSFER_KEEPON;
                }
            }
            break;
        case APPK_PAGE_DOWN:
            {
                _file_browser_list_change_start();
                uint32_t sel;
                GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &sel);
                if (_file_browser_listview_get_count() == sel+1)
                {
                    sel = 0;
                    GUI_SetProperty(LISTVIEW_FILE_BROWSER, "select", &sel);
                    return  EVENT_TRANSFER_STOP;
                }
                else
                {
                    return  EVENT_TRANSFER_KEEPON;
                }
            }
#if MULTIMEDIA_PREVIEW_SUPPORT
        case STBK_LEFT:
        case STBK_RIGHT:
            {
                return  EVENT_TRANSFER_STOP;
            }
        case APPK_PLAY:
            {
                _file_browser_listview_play();
                return  EVENT_TRANSFER_STOP;
            }
            break;
        case APPK_PAUSE:
            {
                _file_browser_listview_pause();
                return  EVENT_TRANSFER_STOP;
            }
            break;
        case APPK_STOP:
            {
                _file_browser_listview_stop(false);
                return  EVENT_TRANSFER_STOP;
            }
           break;
        case APPK_UP:
        case APPK_DOWN:
            _file_browser_list_change_start();
            break;
#endif
        default:
            break;
    }

    return EVENT_TRANSFER_KEEPON;
}


SIGNAL_HANDLER int file_browser_lost_focus(GuiWidget *widget, void *usrdata)
{
	int listview_sel = 0;

	GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", (void*)&listview_sel);
	GUI_SetProperty(LISTVIEW_FILE_BROWSER, "active", &listview_sel);
	GUI_SetProperty(TEXT_PATH2, "format", "static");
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int file_browser_got_focus(GuiWidget *widget, void *usrdata)
{
	int listview_sel = -1;
	GUI_SetProperty(LISTVIEW_FILE_BROWSER, "active", &listview_sel);
	GUI_SetProperty(TEXT_PATH2, "format", "fix_r2l");

	GUI_SetInterface("video_disable", NULL);
    file_browser_list_change_restart();
	return EVENT_TRANSFER_STOP;
}

status_t app_mcentre_hotplug_out(char *hotplug_dev)
{
    bool save_music_player = false;
    if(listview_mode_get() == LISTVIEW_MODE_PARTITION)
    {
        app_file_browser_init();
        GUI_SetProperty(WIN_FILE_BROWSER, "update", NULL);
    }

    if((hotplug_dev != NULL)
            && (strncmp(hotplug_dev, s_mcenter_dev_name, strlen(hotplug_dev)) == 0))
    {
        if((music_play_state() == true) && (strncmp(hotplug_dev, s_playing_music_dev_name, strlen(hotplug_dev))))
        {
            save_music_player = true;
        }

        if(!save_music_player)
        {
            play_av_by_stop();
            memset(s_playing_music_dev_name, 0, sizeof(s_playing_music_dev_name));
            clean_music_file_no_bak();
            music_status_init();
        }
        if(listview_mode_get() == LISTVIEW_MODE_FILE)
        {
            if((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW)))
            {
                GUI_SetProperty("music_view_text_name1", "string", NULL);
            }

            if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
            {
                GUI_EndDialog("after win_file_browser");
                book_popdlg_recreate();
            }
            else
            {
                GUI_EndDialog("after win_file_browser");
            }
            app_file_browser_init();
            GUI_SetProperty(WIN_FILE_BROWSER, "update", NULL);
        }
    }
    app_update_pop_dlg(WND_POP_BOOK);
    return GXCORE_SUCCESS;
}

status_t app_mcentre_hotplug_in(void)
{
    if(listview_mode_get() == LISTVIEW_MODE_PARTITION)
	{
        app_file_browser_init();
        int usb_partition_num = _file_browser_get_usb_partition_num();
        uint32_t index = 0;
        int samba_num = _file_browser_get_samba_num();
        GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
        if( (index < (usb_partition_num + samba_num)) && ((usb_partition_num + samba_num)>=1)
            && (index >=0 ) )
        {
            _file_browser_update_mcenter_dev_name(index);
        }

	}

	return GXCORE_SUCCESS;
}

status_t app_mcentre_network_plug_out(void)
{
    bool need_update = false;
    if((listview_mode_get() == LISTVIEW_MODE_PARTITION)
            && (_file_browser_get_samba_num() > 0))
    {
        need_update = true;
    }

    if(strstr(s_mcenter_dev_name, "samba") != NULL)
    {
        memset(s_mcenter_dev_name, 0, sizeof(s_mcenter_dev_name));
        _file_browser_stop_player();
        if(listview_mode_get() == LISTVIEW_MODE_FILE)
        {
            GUI_EndDialog("after win_file_browser");
            need_update = true;
        }
    }

    if(need_update == true)
    {
        _file_browser_init();
        GUI_SetProperty(WIN_FILE_BROWSER, "update", NULL);
    }

    return GXCORE_SUCCESS;
}

status_t app_mcentre_hotplug_clean(char *hotplug_dev)
{
    if(listview_mode_get() == LISTVIEW_MODE_PARTITION)
    {
        _file_browser_init();
        GUI_SetProperty(WIN_FILE_BROWSER, "update", NULL);
    }

	if((hotplug_dev != NULL) && (strstr(s_mcenter_dev_name, hotplug_dev) != NULL)
			&& (strcmp(GUI_GetFocusWindow(), WIN_FILE_BROWSER) == 0))
	{
        _file_browser_init();
		GUI_SetProperty(WIN_FILE_BROWSER, "update", NULL);

		//listview sel =0, it will not call listview change,cause focus name not save. 20230213
		if(listview_mode_get() == LISTVIEW_MODE_PARTITION)
		{
			int usb_partition_num = _file_browser_get_usb_partition_num();
			uint32_t index = 0;
			int samba_num = _file_browser_get_samba_num();
			GUI_GetProperty(LISTVIEW_FILE_BROWSER, "select", &index);
			if( (index < (usb_partition_num + samba_num)) && ((usb_partition_num + samba_num)>=1)
				&& (index >=0 ) )
			{
				_file_browser_update_mcenter_dev_name(index);
			}
		}
	}
    app_update_pop_dlg(WND_POP_BOOK);
    return GXCORE_SUCCESS;
}

void app_media_file_menu_exec(void)
{
    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    file_view_set_record_group(FILE_VIEW_GROUP_ALL);
    GUI_CreateDialog(WIN_FILE_BROWSER);
}

void app_media_video_menu_exec(void)
{
    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    file_view_set_record_group(FILE_VIEW_GROUP_MOVIE);
    GUI_CreateDialog(WIN_FILE_BROWSER);
}

void app_media_music_menu_exec(void)
{
    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    file_view_set_record_group(FILE_VIEW_GROUP_MUSIC);
    GUI_CreateDialog(WIN_FILE_BROWSER);
}

void app_media_picture_menu_exec(void)
{
    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    file_view_set_record_group(FILE_VIEW_GROUP_PIC);
    GUI_CreateDialog(WIN_FILE_BROWSER);
}
