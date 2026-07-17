#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_utility.h"
#include "play_manage.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_book.h"
#include "app_channel_info.h"
#include "pmp_psi.h"
#if PARENTAL_LOCK_SUPPORT
#include"app_parent_lock.h"
#endif
#ifdef LINUX_OS
#include <sys/stat.h>
#endif

#define PVR_DEFAULT_NUM 64

#define LISTVIEW_PVR_MEIDA      "listview_pvr_list"
#define TXT_PVR_MEDIA_INFO_1 "text_pvr_midia_info_1"
#define TXT_PVR_MEDIA_INFO_2 "text_pvr_midia_info_2"
#define TXT_PVR_MEDIA_INFO_3 "text_pvr_midia_info_3"

#define IMG_DEL_TIP "img_pvr_media_tip_red"
#define TEXT_DEL_TIP "text_pvr_media_tip_red"
#define IMG_DEL_ALL_TIP "img_pvr_media_tip_blue"
#define TEXT_DEL_ALL_TIP "text_pvr_media_tip_blue"

#define IMG_SORT_TIP  "img_pvr_media_tip_yellow"
#define TEXT_SORT_TIP "text_pvr_media_tip_yellow"

#if MULTIMEDIA_PREVIEW_SUPPORT
#define IMG_PVR_MEDIA_SIGNAL "img_pvr_media_signal"
#define TXT_PVR_MEDIA_SIGNAL "txt_pvr_media_signal"
#define IMG_MINI_PLAY        "img_pvr_media_play"
#define IMG_MINI_PAUSE        "img_pvr_media_pause"
#define IMG_MINI_STOP        "img_pvr_media_stop"
#define PVR_MEDIA_SLID      "pvr_media_sliderbar"
#define PVR_MEDIA_START_TIME "pvr_media_text_start_time"
#define PVR_MEDIA_STOP_TIME "pvr_media_text_stop_time"
#define PLAY_FOCUS_IMG		"s_mpicon_play_focus"
#define PLAY_UNFOCUS_IMG		"s_mpicon_play_unfocus"
#define PAUSE_FOCUS_IMG		"s_mpicon_pause_focus"
#define PAUSE_UNFOCUS_IMG	"s_mpicon_pause_unfocus"
#define PLAY_MOVIE_IMG      "MP_ICON_MOVIE_PLAY"
#define PLAY_RADIO_IMG      "MP_ICON_RADIO_PLAY"
#define PLAY_ERROR_IMG      "MP_ICON_ERROR_PLAY"
#endif

typedef struct
{
    uint64_t size;
    time_t time;
}PvrFileInfo;

typedef struct
{
	char			path[500];
	char			dev[16];
	int			nents;
	GxDirent*	ents;
}DirPara;

static struct
{
    int total;
    int *record;
}s_pvr_media_map = {0};

HotplugPartition s_pvr_partition;

static DirPara s_pvr_dir;
static bool s_pvr_on_fullscreen = false;
static char s_pvr_dev_name[32] = {0};
#if MULTIMEDIA_PREVIEW_SUPPORT
static event_list* s_pvrmedia_list_timer = NULL;
static event_list* s_pvrmedia_playtime_timer = NULL;
static play_movie_ctrol_state s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_STOP;
static status_t _pvr_media_for_mini_mediastop(bool exit_flag);
static int _pvr_media_up_down_timer(void* data);
extern void play_get_psi_info(char* path,PmpPsi* psi_info);
#endif

static int pvrlist_sort_mode = 0; // 0:date/time, 1:name
static char* pvrlist_sort_str[]=
{
	"Date/Time",
	"Name",
};
#define PVRLIST_SORT_NUM (sizeof(pvrlist_sort_str)/sizeof(char *))

static int app_pvrlist_date_cmp(const void *a, const void *b)
{
    GxDirent *file_1 = (GxDirent*)a;
    GxDirent *file_2 = (GxDirent*)b;
    int ret = 0;

    if((file_1 == NULL) || (file_2 == NULL))
        return 0;

    if(file_1->fmtime > file_2->fmtime)
        ret = -1;
    else if(file_1->fmtime < file_2->fmtime)
        ret = 1;

    return ret;
}
static int app_pvrlist_name_cmp(const void *a, const void *b)
{
    GxDirent *file_1 = (GxDirent*)a;
    GxDirent *file_2 = (GxDirent*)b;

    if((file_1 == NULL) || (file_2 == NULL))
        return 0;

    return strcasecmp(file_1->fname, file_2->fname);
}

extern void app_free_dir_ent(GxDirent **ents, int *nents);
/////free result extern/////
static char* _get_dvr_dir_path(int index)
{
    char *dir_path = NULL;
    int dir_len=1;
    int file_len;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return NULL;
    }

    file_len = strlen(s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
    dir_len = file_len - PVR_FIX_SIFFIX_LEN;//xxx.ts.dvr
    dir_len += 1; // '/'
    dir_len += strlen(s_pvr_dir.path);
    dir_len += 1; // '\0'

    dir_path = (char*)GxCore_Malloc(dir_len * sizeof(char));
    if(dir_path != NULL)
    {
        memset(dir_path, 0, dir_len);
        strcpy(dir_path, s_pvr_dir.path);
        strcat(dir_path, "/");
        strncat(dir_path, s_pvr_dir.ents[s_pvr_media_map.record[index]].fname, file_len - PVR_FIX_SIFFIX_LEN);
    }
    return dir_path;
}

static char* _get_dvr_dir_path_by_file(const char *fname)
{
    char *dir_path = NULL;
    int dir_len=1;
    int file_len;

    if(fname == NULL)
        return NULL;

    file_len = strlen(fname);
    dir_len = file_len - PVR_FIX_SIFFIX_LEN;//xxx.ts.dvr
    dir_len += 1; // '/'
    dir_len += strlen(s_pvr_dir.path);
    dir_len += 1; // '\0'

    dir_path = (char*)GxCore_Malloc(dir_len * sizeof(char));
    if(dir_path != NULL)
    {
        memset(dir_path, 0, dir_len);
        strcpy(dir_path, s_pvr_dir.path);
        strcat(dir_path, "/");
        strncat(dir_path, fname, file_len - PVR_FIX_SIFFIX_LEN);
    }
    return dir_path;
}

static status_t _get_pvr_media_info(int index, PvrFileInfo *Info)
{
    char *dvr_dir_path = NULL;
    DirPara dvr_dir_para;
    int i;
    status_t ret = GXCORE_ERROR;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return GXCORE_ERROR;
    }

    dvr_dir_path = _get_dvr_dir_path(index);
    if(dvr_dir_path != NULL)
    {
        memset(&dvr_dir_para, 0, sizeof(dvr_dir_para));
        dvr_dir_para.nents  = GxCore_GetDir(dvr_dir_path, &(dvr_dir_para.ents), PVR_MEDIE_SUFFIX);
        GxCore_Free(dvr_dir_path);
        dvr_dir_path = NULL;

        if(dvr_dir_para.nents > 0)
        {
            Info->size = 0;
            for(i = 0; i < dvr_dir_para.nents; i++)
            {
                Info->size += dvr_dir_para.ents[i].fsize;
            }
            Info->time = dvr_dir_para.ents[i-1].fmtime;
            ret = GXCORE_SUCCESS;
        }
        app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
    }

    return ret;
}


static int _check_pvr_file_is_valid(GxDirent ent)
{
    char *dvr_dir_path = NULL;
    DirPara dvr_dir_para;

    if(NULL == ent.fname || ent.ftype != GX_FILE_REGULAR)
        return -1;

    dvr_dir_path = _get_dvr_dir_path_by_file(ent.fname);
    if(dvr_dir_path != NULL)
    {
        memset(&dvr_dir_para, 0, sizeof(dvr_dir_para));
        dvr_dir_para.nents  = GxCore_GetDir(dvr_dir_path, &(dvr_dir_para.ents), PVR_MEDIE_SUFFIX);
        GxCore_Free(dvr_dir_path);
        dvr_dir_path = NULL;

        if(dvr_dir_para.nents > 0)
        {
            app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
            return 0;
        }
        app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
    }

    return -1;
}

static void display_pvr_media_info(PvrFileInfo *Info)
{
    char str[50] = {0};
    char *tm_str = NULL;
    time_t time_sec = 0;

    memset(str, 0, sizeof(str));
    strcpy(str, s_pvr_partition.partition_name);
    strcat(str, " ");
    strcat(str, PVR_DIR_STR);
    GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", str);

    memset(str, 0, sizeof(str));
    play_av_display_size_str(Info->size, str);
    GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", str);

    memset(str, 0, sizeof(str));
    time_sec = get_display_time_by_timezone(Info->time);
    tm_str = app_time_to_format_str(time_sec);
    if(tm_str != NULL)
    {
        strcpy(str, tm_str);
        GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", str);
        GxCore_Free(tm_str);
    }
}


static void _pvr_media_display_file_info(void)
{
    int sel;
    PvrFileInfo file_info;

    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

    memset(&file_info, 0, sizeof(PvrFileInfo));
    if( _get_pvr_media_info(sel, &file_info) == GXCORE_SUCCESS)
    {
        display_pvr_media_info(&file_info);
    }
	else
	{
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_ID_NO_PVR_FILE);
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
	}
    return;
}

#ifdef FILE_EDIT_VALID
static status_t _delete_pvr_media(int index)
{
    status_t ret = GXCORE_SUCCESS;
    int i = 0;
    char *path = NULL;
    int path_len = 0;
    DirPara dvr_dir_para;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return GXCORE_ERROR;
    }

    path = _get_dvr_dir_path(index);
#ifdef LINUX_OS
    struct stat st = {0};
    if(path == NULL || lstat(path, &st) < 0)
    {
        return GXCORE_ERROR;
    }

    if((S_ISDIR(st.st_mode) && (access(path, W_OK) < 0)) || (!S_ISDIR(st.st_mode)))
    {
        return GXCORE_ERROR;
    }
#else
    if(path == NULL)
    {
        return GXCORE_ERROR;
    }
#endif

    memset(&dvr_dir_para, 0, sizeof(dvr_dir_para));
    dvr_dir_para.nents = GxCore_GetDir(path, &(dvr_dir_para.ents), NULL);

    path_len = strlen(path);
    if(dvr_dir_para.nents > 0)
    {
        char* sub_path = NULL;
        int sub_path_len = 0;
        for(i = 0; i < dvr_dir_para.nents; i++)
        {
            sub_path_len = path_len;
            sub_path_len += 1; // '/'
            sub_path_len += strlen(dvr_dir_para.ents[i].fname);
            sub_path_len += 1; // '\0'

            sub_path = (char*)GxCore_Malloc(sub_path_len);
            if(sub_path != NULL)
            {
                memset(sub_path, 0, sub_path_len);
                strcpy(sub_path, path);
                strcat(sub_path, "/");
                strcat(sub_path, dvr_dir_para.ents[i].fname);
                ret = GxCore_FileDelete(sub_path);
                GxCore_Free(sub_path);
                sub_path = NULL;
                if (ret == GXCORE_ERROR)
                {
                    return GXCORE_ERROR;
                }
            }
        }
    }

    app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
    if(GxCore_FileExists(path) == GXCORE_FILE_UNEXIST)
    {
        ret = GXCORE_SUCCESS;
    }
    else
    {
        ret = GxCore_Rmdir(path);
    }
    GxCore_Free(path);
    path = NULL;

    if (ret == GXCORE_ERROR)
    {
        return GXCORE_ERROR;
    }

    //////delete xxx.ts.dvr///////
    path_len = strlen(s_pvr_dir.path);
    path_len += 1;// '/'
    path_len += strlen(s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
    path_len += 1; // '\0'

    path = (char*)GxCore_Malloc(path_len);
    if(path == NULL)
    {
        return GXCORE_ERROR;
    }

    memset(path, 0, path_len);
    strcpy(path, s_pvr_dir.path);
    strcat(path, "/");
    strcat(path, s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
    ret = GxCore_FileDelete(path);
    GxCore_Free(path);
    path = NULL;

    if (ret == GXCORE_ERROR)
    {
        return GXCORE_ERROR;
    }

#ifdef MODIFY_FATMOUNT
    extern void modify_disk_mount(void);
    modify_disk_mount();
#endif

    return ret;
}
#endif

static void _play_pvr_media(int index)
{
    explorer_para explorer;
    memset(&explorer, 0, sizeof(explorer_para));
    explorer.path = s_pvr_dir.path;
    explorer.suffix = PVR_INDEX_SUFFIX;
    explorer.nents = s_pvr_dir.nents;
    explorer.ents = s_pvr_dir.ents;
    play_list_init(PLAY_LIST_TYPE_MOVIE, &explorer, s_pvr_media_map.record[index]);
    GUI_CreateDialog(WIN_MOVIE_VIEW);
}

status_t app_pvr_media_update(void)
{
    char partition[PVR_DEV_STR_LEN + 1] = {0};
    int i = 0;
    int num = 0;
    uint32_t dev_len;
    char *p = NULL;
    HotplugPartitionList *partition_list = NULL;
    status_t ret = GXCORE_ERROR;

   if(s_pvr_media_map.record != NULL)
    {
        GxCore_Free(s_pvr_media_map.record);
        s_pvr_media_map.record= NULL;
    }
    memset(&s_pvr_media_map, 0, sizeof(s_pvr_media_map));
    memset(&s_pvr_partition, 0, sizeof(HotplugPartition));

    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if((NULL != partition_list) && (0 < partition_list->partition_num))
    {
        GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_DEV_STR_LEN, PVR_PARTITION);
        for(i = 0; i < partition_list->partition_num; i++)
        {
            if(strcmp(partition, partition_list->partition[i].dev_name) == 0)
            {
                break;
            }
        }
        if(i >= partition_list->partition_num)
        {
            i = 0;
        }
        memcpy(&s_pvr_partition,  &partition_list->partition[i], sizeof(HotplugPartition));

        memset(s_pvr_dev_name, 0, sizeof(s_pvr_dev_name));
        strcpy(s_pvr_dev_name, s_pvr_partition.dev_name);

        dev_len = strlen(s_pvr_dev_name);
        p = s_pvr_dev_name + dev_len - 1;

        if(*p >= '0' && *p <= '9') //remove partition num
            *p = '\0';

        app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
        memset(&s_pvr_dir, 0, sizeof(s_pvr_dir));

		char dir_name[7] = {0};
#ifdef LINUX_OS
		//获取真实的GxPvr目录的名字（主要解决存在的目录名字大小写的差异问题）
		extern status_t app_get_pvr_dir_name(const char *path);
		if(app_get_pvr_dir_name(s_pvr_partition.partition_entry) == GXCORE_ERROR) {
			strcpy(dir_name, PVR_DIR_STR);
		}else{
			GxBus_ConfigGet(PVR_TRUE_DIR_NAME_KEY, dir_name, 6, PVR_TRUE_DIR_NAME);
		}
#else
		strcpy(dir_name, PVR_DIR_STR);
#endif
        strcpy(s_pvr_dir.path, s_pvr_partition.partition_entry);
        strcat(s_pvr_dir.path, dir_name);

        s_pvr_dir.nents = GxCore_GetDir(s_pvr_dir.path, &(s_pvr_dir.ents), PVR_INDEX_SUFFIX);
        if(s_pvr_dir.nents > 0)
        {
            if(pvrlist_sort_mode>0)
            {
                GxCore_SortDir(s_pvr_dir.ents, s_pvr_dir.nents, app_pvrlist_name_cmp);
            }
            else
            {
                GxCore_SortDir(s_pvr_dir.ents, s_pvr_dir.nents, app_pvrlist_date_cmp);
            }
            s_pvr_media_map.record = (int*)GxCore_Malloc(PVR_DEFAULT_NUM*sizeof(int));
            if(NULL !=  s_pvr_media_map.record)
            {
                num = 0;
                for(i = 0; i < s_pvr_dir.nents; i++)
                {
                    if(0 == _check_pvr_file_is_valid(s_pvr_dir.ents[i]))
                    {
                        if(num >= PVR_DEFAULT_NUM && num%PVR_DEFAULT_NUM == 0)
                        {
                            s_pvr_media_map.record = (int*)GxCore_Realloc(s_pvr_media_map.record,(num+PVR_DEFAULT_NUM)*sizeof(int));
                        }
                        s_pvr_media_map.record[num++] = i;
                    }
                }
                s_pvr_media_map.total = num;
            }
            else
            {
                s_pvr_media_map.total = 0;
                ret = GXCORE_ERROR;
            }
        }
        else
        {
            app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
        }

        ret = GXCORE_SUCCESS;
    }

    return ret;
}

static void _create_pvr_media_wnd(bool on_fullscreen)
{
    PopDlg pop = {0};
    bool ok_flag = false;

    if (app_fuse_common_pvr_record_pre_create_dialog(on_fullscreen))
        ok_flag = true;

    if(ok_flag)
    {
        if (on_fullscreen)
        {
            g_AppFullArb.timer_stop();
            app_clean_fullscreen_tip(false, false, true);
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            g_AppPlayOps.program_stop();
        }

        app_fuse_common_pvr_record_create_dialog();
        s_pvr_on_fullscreen = on_fullscreen;
    }
    else
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_POPUP;
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_INSERT_USB;
        popdlg_create(&pop);
    }
}


void app_create_pvr_media_menu(void)
{
    _create_pvr_media_wnd(false);
}

void app_create_pvr_media_fullsreeen(void)
{
    _create_pvr_media_wnd(true);
}

void _pvr_media_quit(void)
{
	if(s_pvr_on_fullscreen == true)
	{
	    GUI_EndDialog("after wnd_full_screen");
	    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	    GUI_SetProperty(WND_FULL_SCREEN,"draw_now",NULL);

	    if (g_AppPlayOps.normal_play.play_total != 0)
	    {
	        app_play_current_prog(PLAY_MODE_POINT);
	    }
	    s_pvr_on_fullscreen = false;
	}
	else
	{
        app_enddialog_wnd(WND_PVR_MEIDA);

	}
}

void _pvr_media_create(void)
{
    int sel = 0;
    int32_t init_value = 0;

    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

    if(s_pvr_media_map.total > 0)
    {
        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
        _pvr_media_display_file_info();
    }
    else
    {
        GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_ID_NO_PVR_FILE);
    }

#ifdef FILE_EDIT_VALID
	GUI_SetProperty(IMG_DEL_TIP, "state","show");
	GUI_SetProperty(TEXT_DEL_TIP, "state","show");
    GUI_SetProperty(IMG_DEL_ALL_TIP, "state","show");
	GUI_SetProperty(TEXT_DEL_ALL_TIP, "state","show");
    if( APP_THEME_CLASSIC_DARK || APP_THEME_CLASSIC_BLUE || APP_THEME_SKY_BLUE){
    GUI_SetProperty(IMG_SORT_TIP, "state","show");
	GUI_SetProperty(TEXT_SORT_TIP, "state","show");
    }
#endif
}

#ifdef FILE_EDIT_VALID
static void pvr_media_info_display_update_after_del(int sel)
{
	if(GUI_CheckDialog(WND_PVR_MEIDA) == GXCORE_SUCCESS)
	{
		GUI_SetProperty(LISTVIEW_PVR_MEIDA, "update_all", NULL);
		if(s_pvr_media_map.total == 0)
		{
			sel = -1;
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_ID_NO_PVR_FILE);
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
            return;
		}
		else if(sel >= s_pvr_media_map.total)
		{
			sel = s_pvr_media_map.total - 1;
            GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
		}
        _pvr_media_display_file_info();
	}
}

static int _delete_pvr_media_poptip_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        int sel = 0;
        status_t result = GXCORE_SUCCESS;
        GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

        PopDlg  pop = {0};
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_DELETING;
        pop.timeout_sec = 100;
        popdlg_create(&pop);
        GUI_SetInterface("flush",NULL);

        result = _delete_pvr_media(sel);
        popdlg_destroy();
        if (result == GXCORE_SUCCESS)
        {
            app_pvr_media_update();
            pvr_media_info_display_update_after_del(sel);
        }
        else
        {
            PopDlg  pop = {0};
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_UNBLOCK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_DEL_FAIL;
            pop.timeout_sec = 2;
            popdlg_create(&pop);
        }
    }
    return 0;
}

static int _delete_failed_pop_cb(PopDlgRet ret)
{
    app_pvr_media_update();
    pvr_media_info_display_update_after_del(0);

    return 0;
}

static int _delete_all_pvr_media_poptip_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        int sel = 0;
        status_t result = GXCORE_SUCCESS;
        int total = s_pvr_media_map.total;

        PopDlg  pop = {0};
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_DELETING;
        pop.timeout_sec = 1000;
        popdlg_create(&pop);
        GUI_SetInterface("flush",NULL);

        while(total)
        {
            result = _delete_pvr_media(sel);
            if (result == GXCORE_SUCCESS)
            {
                total--;
                sel++;
            }
            else
            {
                break;
            }
        }

        if (result == GXCORE_SUCCESS)
        {
            popdlg_destroy();
            app_pvr_media_update();
            pvr_media_info_display_update_after_del(0);
        }
        else
        {
            PopDlg  pop = {0};
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_UNBLOCK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_DEL_FAIL;
            pop.timeout_sec = 2;
            pop.exit_cb = _delete_failed_pop_cb;
            popdlg_create(&pop);
        }
    }
    return 0;
}
#endif

#if MULTIMEDIA_PREVIEW_SUPPORT
static void _pvr_media_set_play_pause_stop(bool play_flag, bool pause_flag, bool stop_flag)
{
    char *pfocus = NULL;
    char *punfocus = NULL ;
    if ( APP_THEME_CLASSIC_BLUE ){
        pfocus   = "MP_ICON_STOP_YELLOW" ;
        punfocus = "MP_ICON_STOP" ;
    }
    else{
        pfocus   = "s_mpicon_stop_focus" ;
        punfocus = "s_mpicon_stop_unfocus" ;
    }

    if(play_flag)
        GUI_SetProperty(IMG_MINI_PLAY, "img", PLAY_FOCUS_IMG);
    else
        GUI_SetProperty(IMG_MINI_PLAY, "img", PLAY_UNFOCUS_IMG);

    if(pause_flag)
        GUI_SetProperty(IMG_MINI_PAUSE, "img", PAUSE_FOCUS_IMG);
    else
        GUI_SetProperty(IMG_MINI_PAUSE, "img", PAUSE_UNFOCUS_IMG);

    if(stop_flag)
        GUI_SetProperty(IMG_MINI_STOP, "img", pfocus);
    else
        GUI_SetProperty(IMG_MINI_STOP, "img", punfocus);
}

static void _pvr_media_set_video_state(bool video_flag ,bool state_flag, char* image)
{
    if(state_flag)
    {
        GUI_SetProperty(TXT_PVR_MEDIA_SIGNAL, "state", "show");
        if(image)
        {
            GUI_SetProperty(IMG_PVR_MEDIA_SIGNAL, "state", "show");
            GUI_SetProperty(IMG_PVR_MEDIA_SIGNAL, "img", image);
        }
    }
    else
    {
        GUI_SetProperty(TXT_PVR_MEDIA_SIGNAL, "state", "osd_trans_hide");
        GUI_SetProperty(IMG_PVR_MEDIA_SIGNAL, "state", "osd_trans_hide");
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

static int _pvrmedia_lock_play_cb(PopPwdRet *ret, void *usr_data)
{
    PlayerProgInfo *player_info = app_player_prog_info_get();
    uint32_t index = 0;
    char* path = NULL;
    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &index);
    if(ret->result == PASSWD_OK)
    {
        path = explorer_static_path_strcat(s_pvr_dir.path,s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
        if(GxPlayer_MediaGetProgInfoByUrl(path, player_info) != 0)
        {
            return -1;
        }
        GUI_SetInterface("flush", NULL);
        if(player_info->is_radio == 1)
        {
            _pvr_media_set_video_state(false,true,PLAY_RADIO_IMG);
        }
        else
        {
            _pvr_media_set_video_state(true,false,NULL);
        }
        play_av_window_set(TXT_PVR_MEDIA_SIGNAL);
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
        passwd_dlg.passwd_exit_cb = _pvrmedia_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
    }
    return 0;
}

#if PARENTAL_LOCK_SUPPORT
int app_pvrmedia_parent_lock_play(void)
{
    PasswdDlg passwd_dlg;
    play_av_by_stop();
    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.str = STR_ID_PARENT_GUID;
    passwd_dlg.passwd_exit_cb = _pvrmedia_lock_play_cb;
    passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
    passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
    passwd_dlg_create(&passwd_dlg);
    return 0;
}
#endif

static int _pvr_media_for_mini_mediaplay(void)
{
    PlayerProgInfo *player_info = app_player_prog_info_get();
    uint32_t sel = 0;
    char *path = NULL;
    PmpPsi psi_info = {0};
    _pvr_media_set_play_pause_stop(false,true,false);
    if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PAUSE)
    {
        play_av_by_resume();
        s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_PLAY;
        return 0;
    }
    if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PLAY)
    {
        return 0;
    }
    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
        return -1;
    path = explorer_static_path_strcat(s_pvr_dir.path,s_pvr_dir.ents[s_pvr_media_map.record[sel]].fname);
    play_get_psi_info(path,&psi_info);
    if(psi_info.private_info.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        PasswdDlg passwd_dlg;
        play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _pvrmedia_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
        s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_PLAY;
        return 0;
    }
    if(GxPlayer_MediaGetProgInfoByUrl(path, player_info) != 0)
    {
        return -1;
    }
    if(player_info->is_radio == 1)
    {
        _pvr_media_set_video_state(false,true,PLAY_RADIO_IMG);
    }
    else
    {
        _pvr_media_set_video_state(true,false,NULL);
    }
    play_av_window_set(TXT_PVR_MEDIA_SIGNAL);
    play_av_by_url(path,0);
#if PARENTAL_LOCK_SUPPORT
    app_pvr_file_parental_rating_start(1,psi_info.ts_id,psi_info.service_id);
#endif
    s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_PLAY;
    return 0;
}

static status_t _pvr_media_for_mini_mediapause(void)
{
    if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PLAY)
    {
        _pvr_media_set_play_pause_stop(true,false,false);
        play_av_by_pause();
        s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_PAUSE;
    }
    else if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PAUSE)
    {
        _pvr_media_set_play_pause_stop(false,true,false);
        play_av_by_resume();
        s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_PLAY;
    }
    return GXCORE_SUCCESS;
}

static status_t _pvr_media_for_mini_mediastop(bool exit_flag)
{
#if PARENTAL_LOCK_SUPPORT
    app_pvr_file_parental_rating_stop();
#endif
    int cur_sliderbar = 0;
    GUI_SetProperty(PVR_MEDIA_SLID, "value", &cur_sliderbar);
    GUI_SetProperty(PVR_MEDIA_START_TIME, "string", "00:00:00");
    _pvr_media_set_video_state(false,true,NULL);
    if(exit_flag == false)
    {
        _pvr_media_set_play_pause_stop(false,false,true);
    }
    else
    {
        _pvr_media_set_play_pause_stop(false,false,false);
    }
    if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PLAY || s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_PAUSE || exit_flag == true)
    {
        play_av_by_stop();
        play_av_window_set(TXT_PVR_MEDIA_SIGNAL);
        s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_STOP;
    }
    return  GXCORE_SUCCESS;
}

static int _pvr_media_up_down_timer(void* data)
{
    PlayerProgInfo *player_info = app_player_prog_info_get();
    char str_size[20] = {0};
    char *path = NULL;
    uint32_t sel = 0;
    APP_TIMER_REMOVE(s_pvrmedia_list_timer);
    APP_TIMER_REMOVE(s_pvrmedia_playtime_timer);
    _pvr_media_for_mini_mediastop(true);
    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        GUI_SetProperty(PVR_MEDIA_STOP_TIME, "string", "00:00:00");
        return -1;
    }
    path = explorer_static_path_strcat(s_pvr_dir.path,s_pvr_dir.ents[s_pvr_media_map.record[sel]].fname);
    if(GxPlayer_MediaGetProgInfoByUrl(path, player_info) != 0)
    {
        GUI_SetProperty(PVR_MEDIA_STOP_TIME, "string", "00:00:00");
        _pvr_media_set_video_state(false,true,PLAY_ERROR_IMG);
        return -1;
    }
    app_time_ms_to_edit_str(player_info->duration, str_size, 20);
    GUI_SetProperty(PVR_MEDIA_STOP_TIME, "string", str_size);
    if(player_info->is_radio == 1)
    {
        _pvr_media_set_video_state(false,true,PLAY_RADIO_IMG);
    }
    else
    {
        _pvr_media_set_video_state(false,true,PLAY_MOVIE_IMG);
    }
    _pvr_media_set_play_pause_stop(true,false,false);
    return 0;
}

static int _pvr_media_play_time_set(void* data)
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
        GUI_SetProperty(PVR_MEDIA_SLID, "value", &cur_sliderbar);
        app_time_ms_to_edit_str(cur_time_ms, buf_tmp, 20);
        GUI_SetProperty(PVR_MEDIA_START_TIME, "string", buf_tmp);
    }
    app_update_pop_dlg(WND_POP_OPTION_LIST);
    return 0;
}

void app_pvr_media_quit(void)
{
    APP_TIMER_REMOVE(s_pvrmedia_list_timer);
    APP_TIMER_REMOVE(s_pvrmedia_playtime_timer);
    _pvr_media_for_mini_mediastop(true);
    play_av_window_set_full_screen();
    _pvr_media_quit();
}

static int pvr_media_list_status_report(GxMsgProperty_PlayerStatusReport* player_status)
{
	APP_CHECK_P(player_status, 1);

	switch(player_status->status)
	{
		case PLAYER_STATUS_PLAY_END:
            APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
            APP_TIMER_ADD(s_pvrmedia_playtime_timer, _pvr_media_play_time_set, 200, TIMER_REPEAT);
			break;

		case PLAYER_STATUS_ERROR:
            _pvr_media_set_video_state(false,true,PLAY_ERROR_IMG);
			break;

		default:
			break;
	}
	return 0;
}

static int32_t _app_pvr_media_sort_poplist_cb(int32_t ret_sel, unsigned short key)
{
    if ( ret_sel !=  pvrlist_sort_mode )
    {
        pvrlist_sort_mode = ret_sel ;
        app_pvr_media_update();
        _pvr_media_display_file_info();
    }

    return 0;
}

SIGNAL_HANDLER  int app_pvr_media_service(const char* widgetname, void *usrdata)
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
			pvr_media_list_status_report(player_status);
			break;

		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_got_focus(GuiWidget *widget, void *usrdata)
{
    if(s_pvr_mini_mediaplay_state == PLAY_MOVIE_CTROL_STOP)
    {
        APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_create(GuiWidget *widget, void *usrdata)
{
    _pvr_media_create();
    APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int sel = 0;


    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
         switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
         {
             case VK_BOOK_TRIGGER:
                 APP_TIMER_REMOVE(s_pvrmedia_playtime_timer);
                 APP_TIMER_REMOVE(s_pvrmedia_list_timer);
                 _pvr_media_for_mini_mediastop(true);
                 play_av_window_set_full_screen();
                 GUI_EndDialog("after wnd_full_screen");
                 break;

             case STBK_EXIT:
             case STBK_MENU:
                app_pvr_media_quit();
                 break;

             case STBK_RED:
                 {
#ifdef FILE_EDIT_VALID
                     if(s_pvr_media_map.total == 0)
                         break;
                     _pvr_media_for_mini_mediastop(true);
                     PopDlg  pop = {0};
                     pop.type = POP_TYPE_YES_NO;
                     pop.mode = POP_MODE_UNBLOCK;
                     pop.str = STR_ID_SURE_DELETE;
                     pop.exit_cb = _delete_pvr_media_poptip_cb;
                     popdlg_create(&pop);
#endif
                 }
                 break;
             case STBK_BLUE:
                 {
#ifdef FILE_EDIT_VALID
                     if(s_pvr_media_map.total == 0)
                         break;
                      _pvr_media_for_mini_mediastop(true);
                     PopDlg  pop = {0};
                     pop.type = POP_TYPE_YES_NO;
                     pop.mode = POP_MODE_UNBLOCK;
                     pop.str = STR_ID_SURE_DELETE_ALL;
                     pop.exit_cb = _delete_all_pvr_media_poptip_cb;
                     popdlg_create(&pop);
#endif
                 }
                 break;

             case STBK_YELLOW:
                 {
                     if(s_pvr_media_map.total == 0)
                         break;

                     PopList pop_list;
                     memset(&pop_list, 0, sizeof(PopList));
                     pop_list.title = STR_ID_SORT;
                     pop_list.item_num = PVRLIST_SORT_NUM;
                     pop_list.item_content = pvrlist_sort_str;
                     pop_list.sel = pvrlist_sort_mode;
                     pop_list.show_num= false;
                     pop_list.mode = POP_MODE_UNBLOCK;
                     pop_list.exit_cb = _app_pvr_media_sort_poplist_cb;
                     poplist_create(&pop_list);
                 }
                 break;

             case STBK_OK:
                 if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
                     return EVENT_TRANSFER_STOP;
                 APP_TIMER_REMOVE(s_pvrmedia_playtime_timer);
                 APP_TIMER_REMOVE(s_pvrmedia_list_timer);
                 _pvr_media_for_mini_mediastop(true);
                 s_pvr_mini_mediaplay_state = PLAY_MOVIE_CTROL_STOP;
                 play_av_window_set_full_screen();
                 GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                 _play_pvr_media(sel);
                 break;

            default:
                 break;
         }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t sel = 0;

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
                    APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
                    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_pvr_media_map.total - 1;
                        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_UP;
                    }
                    break;

                case STBK_PAGE_DOWN:
                    APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
                    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                    if (sel == s_pvr_media_map.total - 1)
                    {
                        sel = 0;
                        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_DOWN;
                    }
                    break;
             case STBK_LEFT:
             case STBK_RIGHT:
                    {
                        return EVENT_TRANSFER_STOP;
                    }
             case STBK_PLAY:
                    if(s_pvr_media_map.total != 0)
                    {
                        _pvr_media_for_mini_mediaplay();
                    }
                    break;
             case STBK_PAUSE:
                    if(s_pvr_media_map.total != 0)
                    {
                        _pvr_media_for_mini_mediapause();
                    }
                 break;
             case STBK_REC_STOP:
                 if(s_pvr_media_map.total != 0)
                 {
                     _pvr_media_for_mini_mediastop(false);
                 }
                 break;
             case STBK_UP:
             case STBK_DOWN:
                  APP_TIMER_ADD(s_pvrmedia_list_timer, _pvr_media_up_down_timer, 300, TIMER_REPEAT);
                 break;
                default:
                    break;
            }
    }
    return ret;
}

#else
void app_pvr_media_quit(void)
{
    _pvr_media_quit();
}

static int32_t _app_pvr_media_sort_poplist_cb(int32_t ret_sel, unsigned short key)
{
    if ( ret_sel !=  pvrlist_sort_mode )
    {
        pvrlist_sort_mode = ret_sel ;
        app_pvr_media_update();
        _pvr_media_display_file_info();
    }

    return 0;
}

SIGNAL_HANDLER int app_pvr_media_create(GuiWidget *widget, void *usrdata)
{
    _pvr_media_create();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int sel = 0;


    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
         switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
         {
             case VK_BOOK_TRIGGER:
                 GUI_EndDialog("after wnd_full_screen");
                 break;

             case STBK_EXIT:
             case STBK_MENU:
                 app_pvr_media_quit();
                 break;

             case STBK_RED:
                 {
#ifdef FILE_EDIT_VALID
                     if(s_pvr_media_map.total == 0)
                         break;

                     PopDlg  pop = {0};
                     pop.type = POP_TYPE_YES_NO;
                     pop.mode = POP_MODE_UNBLOCK;
                     pop.str = STR_ID_SURE_DELETE;
                     pop.exit_cb = _delete_pvr_media_poptip_cb;
                     popdlg_create(&pop);
#endif
                 }
                 break;
             case STBK_BLUE:
                 {
#ifdef FILE_EDIT_VALID
                     if(s_pvr_media_map.total == 0)
                         break;

                     PopDlg  pop = {0};
                     pop.type = POP_TYPE_YES_NO;
                     pop.mode = POP_MODE_UNBLOCK;
                     pop.str = STR_ID_SURE_DELETE_ALL;
                     pop.exit_cb = _delete_all_pvr_media_poptip_cb;
                     popdlg_create(&pop);
#endif
                 }
                 break;

             case STBK_YELLOW:
                 {
                     if(s_pvr_media_map.total == 0)
                         break;

                     PopList pop_list;
                     memset(&pop_list, 0, sizeof(PopList));
                     pop_list.title = STR_ID_SORT;
                     pop_list.item_num = PVRLIST_SORT_NUM;
                     pop_list.item_content = pvrlist_sort_str;
                     pop_list.sel = pvrlist_sort_mode;
                     pop_list.show_num= false;
                     pop_list.mode = POP_MODE_UNBLOCK;
                     pop_list.exit_cb = _app_pvr_media_sort_poplist_cb;
                     poplist_create(&pop_list);
                 }
                 break;

             case STBK_OK:
                 if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
                     return EVENT_TRANSFER_STOP;
                 GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                 _play_pvr_media(sel);
                 break;

            default:
                 break;
         }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t sel = 0;

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
                    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_pvr_media_map.total - 1;
                        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_UP;
                    }
                    break;

                case STBK_PAGE_DOWN:
                    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                    if (sel == s_pvr_media_map.total - 1)
                    {
                        sel = 0;
                        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_DOWN;
                    }
                    break;
                default:
                    break;
            }
    }
    return ret;
}
#endif

SIGNAL_HANDLER int app_pvr_media_destroy(GuiWidget *widget, void *usrdata)
{
    int32_t init_value = 0;
#if MULTIMEDIA_PREVIEW_SUPPORT
    APP_TIMER_REMOVE(s_pvrmedia_list_timer);
    APP_TIMER_REMOVE(s_pvrmedia_playtime_timer);
#endif
    pmpset_exit();
    if(s_pvr_media_map.record != NULL)
    {
        GxCore_Free(s_pvr_media_map.record);
        s_pvr_media_map.record= NULL;
    }
    s_pvr_media_map.total = 0;

    app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
    memset(&s_pvr_dir, 0, sizeof(s_pvr_dir));

    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_get_total(GuiWidget *widget, void *usrdata)
{
    return s_pvr_media_map.total;
}

SIGNAL_HANDLER int app_pvr_media_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;

    if(s_pvr_media_map.record == NULL || item->sel < 0 || item->sel >= s_pvr_media_map.total)
        return GXCORE_ERROR;

    //col-0: channel name
    item->image = NULL;
    item->string = s_pvr_dir.ents[s_pvr_media_map.record[item->sel]].fname;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_change(GuiWidget *widget, void *usrdata)
{
    _pvr_media_display_file_info();
    return EVENT_TRANSFER_STOP;
}

status_t app_pvr_media_hotplug(char *hotplug_dev)
{
	int i = 0;
    char partition[PVR_DEV_STR_LEN + 1] = {0};
    HotplugPartitionList* partition_list = NULL;

    if ((s_pvr_dev_name != NULL)
            && (hotplug_dev != NULL)
            && strncmp(s_pvr_dev_name, hotplug_dev, strlen(hotplug_dev)) == 0)
    {
        if (GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS) {
            play_av_by_stop();
        }
        app_pvr_media_quit();

        partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
        if (partition_list != NULL) {
            GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_DEV_STR_LEN,  PVR_PARTITION);
            for (i = 0; i < partition_list->partition_num; i++) {
                if (strcmp(partition, partition_list->partition[i].dev_name) == 0) {
                    break;
                }
            }

            if ((partition_list->partition_num > 0) && (i >= partition_list->partition_num)) {
                if (partition_list->partition[0].dev_name == NULL) {
                    return GXCORE_ERROR;
                }
                GxBus_ConfigSet(PVR_PARTITION_KEY, partition_list->partition[0].dev_name);
            }
        }
    }

    return GXCORE_SUCCESS;
}
