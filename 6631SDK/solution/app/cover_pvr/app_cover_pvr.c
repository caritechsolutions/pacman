#include "app.h"
#include "app_config.h"
#include "module/app_log.h"
#include "play_movie.h"
#if PVR_RECORDING_POSTER_STYLE
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_str_id.h"
#include "app_utility.h"
#include "gui_core.h"
#include "play_manage.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_number.h"
#include "module/app_pvr.h"
#include "app_cover_pvr.h"
#ifdef LINUX_OS
#include <sys/stat.h>
#endif

// wnd_cover_pvr paras
#define MORE_GROUP_SUPPORT 0
#define LEFT_RIGHT_SERIAL_MODE 1
#define FOCUS_GROUP_DEFAULT 0
#define TIMER_FLUSH_LOGO 1

#define PVR_DEFAULT_NUM 64
#define PVR_CONTENT_TYPE_MAX 1000
#define PVR_ITEMS_PER_PAGE 6
#if MORE_GROUP_SUPPORT
#define PVR_GROUPS_PER_PAGE 5
#else
#define PVR_GROUPS_PER_PAGE 6
#endif
#define PVR_ITEMS_COL_NUMS 2
#define PVR_ITEMS_ROW_NUMS 3

#define ITEM_COMMON_SIZE 32
#ifdef LINUX_OS
#define CACHE_LOGO_COMMON "/tmp/record_logo%d.jpg"
#else
#define CACHE_LOGO_COMMON "/mnt/record_logo%d.jpg"
#endif
#define ITEM_KEY_COMMON "record_logo_key%d"
#define RECORD_CHKEY "record_chkey"

#define ITEM_FOCUS_IMG "cover_pvr_focus"
#define ITEM_UNFOCUS_IMG "cover_pvr_unfocus"
#define GROUP_FOCUS_IMG "cover_pvr_group_focus"
#define GROUP_SELECT_IMG "cover_pvr_group_select"

#define IDLE_BG_COLOR "[window_bg_color,window_bg_color,window_bg_color]"
#define IDLE_BG2_COLOR "[#212121,#212121,#212121]"

typedef struct {
    int total;
    char **names;
} FileFilter;

typedef enum { FILE_IDLE = 0, FILE_NODATA = (1 << 0), FILE_REMOVED = (1 << 1) } FileFlag;

typedef struct {
    uint64_t size;
    time_t time;
    int flag;
} FileAttr;

typedef struct {
    time_t start;
    time_t finish;
    time_t recstart;
    time_t duration;
    int rate;
    int nfilters;
    int *filters;
    char *title;
    char *desc;
    char *chname;
    int from_freescan;
#if PVR_PREVIEW_SUPPORT
    time_t preview_offset;
    char *preview_path;
    int preview_flag;
#endif
} FileInfo;

typedef struct {
    int nents;
    GxDirent *ents;
    FileAttr *attrs;
    FileInfo *infos;
    int total;
} DirPara;

typedef enum { LOGO_IDLE = 0, LOGO_ADD, LOGO_COPY, LOGO_FIN } LogoState;

typedef struct {
    LogoState state;
    char *name;
} LogoPara;

typedef enum {
    AREA_ITEM = 0,
    AREA_GROUP,
#if MORE_GROUP_SUPPORT
    AREA_MORE,
#endif
} AreaSel;

typedef struct {
    handle_t mutex;
    uint8_t first_in_flag;
    uint8_t rebuild_flag;
    uint8_t thread_build_run;
    uint8_t build_stop_rq;

    uint8_t thread_cache_run;
    uint8_t cache_stop_rq;
    uint8_t logo_copy_rq;
    uint8_t logo_stop_rq;

    char *index_path;
    struct cover_pvr_epg_record *record;
    int record_num;
    char **extra_path;
    int extra_num;
    LogoPara logo_para[PVR_ITEMS_PER_PAGE];
} CoverPvrMgr;

typedef struct {

    bool reverse_flag;
    bool chlogo_bg_flag;

    const char *wnd_name;
    const char *img_menu_logo;
    const char *group_item_common;
    const char *item_back_common;
    const char *item_logo_common;
#if PVR_PREVIEW_SUPPORT
    const char *item_preview_common;
#endif
    const char *item_title_common;
    const char *item_time_common;
    const char *item_ch_common;

    const char *text_item_filter;
    const char *text_item_page;

    const char *text_item_duration;
    const char *text_item_name;
    const char *text_item_parent;
    const char *text_item_genres;
    const char *text_item_brief;

    const char *text_ch_name;
    const char *text_ch_logo;
    const char *img_ch_logo;

    const char *text_item_size;
    const char *text_item_dir;
    const char *text_item_file;

    const char *details_back;
    const char *details_duration;
    const char *details_title;
    const char *details_notepad;
    const char *details_exit;

    const char *img_ok_tip;
    const char *txt_ok_tip;
    const char *text_time;
    const char *text_date;
} CoverPvrWidget;

typedef struct {
    CoverPvrMgr mgr;
    CoverPvrWidget wgt;
    uint8_t screen_flag;
    uint8_t detail_flag;
    event_list *rebuild_timer;
    event_list *logo_timer;
    event_list *systime_timer;
#if PVR_PREVIEW_SUPPORT
    event_list *preview_timer;
    event_list *preview_stop_timer;
#endif

    int filter_sel;
    int cur_index;
    AreaSel area_sel;
    int focus_sel;
    int rfocus_sel;

    char path[PVR_PATH_LEN];
    HotplugPartition partition;
    FileFilter filter;
    DirPara paras;
    int nuses;
    int *uses;
    _ex_number_response_func func;
    char *menu_logo;
    bool inited;
} CoverPvrCtrl;

static CoverPvrCtrl s_cover_pvr_ctrl;

static int _cover_pvr_rebuild_exec(void *userdata);
static int _cover_pvr_update_group_all(int last_sel, int new_sel, AreaSel area_sel);
static void _cover_pvr_details_hide(void);
#if PVR_PREVIEW_SUPPORT
static int _cover_pvr_preview_stop(void *usrdata);
static int _cover_pvr_preview(void *usrdata);
#endif
extern void app_free_dir_ent(GxDirent **ents, int *nents);

char *app_cover_pvr_get_full_name(const char *path, int plen, const char *fname, int flen)
{
    int len = 0;
    char *name = NULL;

    if(!path || !fname)
        return NULL;
    len = plen + flen + 2;
    if((name = GxCore_Calloc(len, 1)) == NULL)
    {
        COVER_PVR_ERR("malloc failed!\n");
        return NULL;
    }
    memcpy(name, path, plen);
    memcpy(name + plen, "/", 1);
    memcpy(name + plen + 1, fname, flen);

    return name;
}

char *app_cover_pvr_get_full_name2(const char *path, const char *fname)
{
    if(!path || !fname)
        return NULL;
    return app_cover_pvr_get_full_name(path, strlen(path), fname, strlen(fname));
}

static char *_get_dvr_dir_path_by_file(const char *fname)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    if(fname == NULL)
        return NULL;
    return app_cover_pvr_get_full_name(ctrl->path, strlen(ctrl->path), fname, strlen(fname) - PVR_FIX_SIFFIX_LEN);
}

static char *_get_dvr_dir_path_by_index(int index)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    const char *fname = NULL;

    if((ctrl->nuses == 0) || (ctrl->uses == NULL))
        return NULL;
    fname = ctrl->paras.ents[ctrl->uses[index]].fname;
    return _get_dvr_dir_path_by_file(fname);
}

static int _file_info_free(FileInfo *info)
{
    if(!info)
        return -1;

    APP_FREE(info->filters);
    APP_FREE(info->title);
    APP_FREE(info->desc);
    APP_FREE(info->chname);
    memset(info, 0, sizeof(FileInfo));

    return 0;
}

static int _cover_pvr_ctrl_release(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int i = 0;

    if(ctrl->inited == false)
    {
        app_log_info("cover pvr ctrl not inited!\n");
        return 0;
    }

    APP_TIMER_REMOVE(ctrl->logo_timer);
    APP_TIMER_REMOVE(ctrl->rebuild_timer);
    APP_TIMER_REMOVE(ctrl->systime_timer);
#if PVR_PREVIEW_SUPPORT
    APP_TIMER_REMOVE(ctrl->preview_timer);
    APP_TIMER_REMOVE(ctrl->preview_stop_timer);
#endif

    memset(ctrl->path, 0, sizeof(ctrl->path));
    memset(&ctrl->partition, 0, sizeof(ctrl->partition));

    for(i = 0; i < ctrl->filter.total; i++)
        APP_FREE(ctrl->filter.names[i]);
    APP_FREE(ctrl->filter.names);
    memset(&ctrl->filter, 0, sizeof(ctrl->filter));

    for(i = 0; i < ctrl->paras.nents; i++)
        _file_info_free(&ctrl->paras.infos[i]);
    APP_FREE(ctrl->paras.infos);
    APP_FREE(ctrl->paras.attrs);
    app_free_dir_ent(&ctrl->paras.ents, &ctrl->paras.nents);
    memset(&ctrl->paras, 0, sizeof(ctrl->paras));

    APP_FREE(ctrl->uses);
    ctrl->nuses = 0;

    APP_FREE(ctrl->menu_logo);
    ctrl->inited = false;

    return 0;
}

static int _cover_pvr_ctrl_filter(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int i, j;

    if(ctrl->filter_sel > ctrl->filter.total)
        ctrl->filter_sel = 0;

    ctrl->nuses = 0;
    if(ctrl->filter_sel == 0)
    {
        for(i = 0; i < ctrl->paras.nents; i++)
        {
            if(ctrl->paras.attrs[i].flag == 0)
                ctrl->uses[ctrl->nuses++] = i;
        }
    }
    else
    {
        int filter_sel = ctrl->filter_sel - 1;
        for(i = 0; i < ctrl->paras.nents; i++)
        {
            if(ctrl->paras.attrs[i].flag == 0)
            {
                for(j = 0; j < ctrl->paras.infos[i].nfilters; j++)
                {
                    if(ctrl->paras.infos[i].filters[j] == filter_sel)
                    {
                        ctrl->uses[ctrl->nuses++] = i;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

static int _copy_record_to_file_info(struct cover_pvr_epg_record *record, FileInfo *info)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int i = 0, j = 0;

    if(!record || !info)
        return -1;

    info->start = record->starttime;
    info->finish = record->finishtime;
    info->recstart = record->recstart;
    info->duration = record->duration;
    if(record->contents_num > 0 && (info->filters = GxCore_Calloc(record->contents_num, sizeof(int))) != NULL)
    {
        for(i = 0; i < record->contents_num; i++)
        {
            for(j = 0; j < ctrl->filter.total; j++)
            {
                if(strcmp(ctrl->filter.names[j], record->contents[i]) == 0)
                {
                    info->filters[info->nfilters++] = j;
                    break;
                }
            }
            if(j == ctrl->filter.total && PVR_CONTENT_TYPE_MAX > ctrl->filter.total)
            {
                if(record->contents[i] && (ctrl->filter.names[j] = GxCore_Strdup(record->contents[i])) != NULL)
                {
                    info->filters[info->nfilters++] = j;
                    ctrl->filter.total++;
                }
            }
        }
    }
    info->rate = record->parental;
    if(record->name)
        info->title = GxCore_Strdup(record->name);
    if(record->description)
        info->desc = GxCore_Strdup(record->description);
    if(record->chname)
        info->chname = GxCore_Strdup(record->chname);
    info->from_freescan = record->from_freescan;
#if PVR_PREVIEW_SUPPORT
    info->preview_offset = record->preview.offset;
    info->preview_flag = record->preview.flag;
    if(record->preview.path)
        info->preview_path = GxCore_Strdup(record->preview.path);
#endif

    return 0;
}

static void _cover_pvr_thread_stop_set(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if(ctrl->mgr.thread_build_run == 1)
        ctrl->mgr.build_stop_rq = 1;
    if(ctrl->mgr.thread_cache_run == 1)
        ctrl->mgr.cache_stop_rq = 1;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}

static bool _cover_pvr_thread_run_check(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    bool flag = false;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if(ctrl->mgr.thread_build_run == 1 || ctrl->mgr.thread_cache_run == 1)
        flag = true;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    return flag;
}

static bool _index_json_build_stop_check(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    bool flag;

    GxCore_MutexLock(ctrl->mgr.mutex);
    flag = (bool)ctrl->mgr.build_stop_rq;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    return flag;
}
static void *_index_json_build_fname_get(const char *info_path)
{
    const char *tmp = NULL;
    const char *base1 = NULL, *base2 = NULL;
    char *fname = NULL;
    int base_len = 0;
    int len = 0;

    if(!info_path)
        return NULL;

    tmp = info_path + strlen(info_path);
    while(--tmp != info_path)
    {
        if(*tmp == '/')
        {
            if(base1 == NULL)
            {
                base1 = tmp;
            }
            else if(base2 == NULL)
            {
                base2 = tmp + 1;
                break;
            }
        }
    }
    if(!base2 || (base_len = base1 - base2) == 0)
        return NULL;

    len = base_len + PVR_FIX_SIFFIX_LEN + 1;
    if((fname = GxCore_Calloc(len, 1)) == NULL)
        return NULL;
    memcpy(fname, base2, base_len);
    memcpy(fname + base_len, PVR_FIX_SIFFIX, PVR_FIX_SIFFIX_LEN);

    return fname;
}

static void _index_json_build_fname_free(void *fname)
{
    APP_FREE(fname);
}

static void _cover_pvr_index_json_build(void *arg)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    struct cover_pvr_epg_record_func record_func;
    char *index_path = NULL;
    struct cover_pvr_epg_record *record = NULL;
    int record_num = 0;
    char **extra_path = NULL;
    int extra_num = 0;
    int i = 0;

    GxCore_ThreadDetach();

    record_func.stop_check = _index_json_build_stop_check;
    record_func.fname_get = _index_json_build_fname_get;
    record_func.fname_free = _index_json_build_fname_free;

    GxCore_MutexLock(ctrl->mgr.mutex);
    index_path = ctrl->mgr.index_path;
    record = ctrl->mgr.record;
    record_num = ctrl->mgr.record_num;
    extra_path = ctrl->mgr.extra_path;
    extra_num = ctrl->mgr.extra_num;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    app_cover_pvr_save_epg_record_all_info(index_path, record, record_num, extra_path, extra_num, &record_func);

    GxCore_MutexLock(ctrl->mgr.mutex);
    if((ctrl->mgr.extra_num > 0 || ctrl->mgr.record_num > 0) && ctrl->mgr.build_stop_rq == 0)
        ctrl->mgr.rebuild_flag = 1;
    APP_FREE(ctrl->mgr.index_path);
    for(i = 0; i < ctrl->mgr.extra_num; i++)
        APP_FREE(ctrl->mgr.extra_path[i]);
    APP_FREE(ctrl->mgr.extra_path);
    app_cover_pvr_free_epg_record_all(&ctrl->mgr.record, &ctrl->mgr.record_num);
    ctrl->mgr.thread_build_run = 0;
    ctrl->mgr.build_stop_rq = 0;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}

typedef struct {
    GxDirent ent;
    FileAttr attr;
    FileInfo info;
} CoverPvrSort;

static int _pvr_cmp(const void *a, const void *b)
{
#if 0  // 开启时先按EPG事件的开始时间排序
    CoverPvrSort *aa = (CoverPvrSort *)a, *bb = (CoverPvrSort *)b;
    int ret = 0;

    if (aa->info.start != bb->info.start)
        return bb->info.start - aa->info.start;
    if (aa->info.title != NULL && bb->info.title != NULL)
        ret = strcmp(bb->info.title, aa->info.title);
    if (ret != 0)
        return ret;
    if (aa->info.start != 0)
        return bb->ent.fmtime - aa->ent.fmtime;

    if (aa->ent.fname != NULL && bb->ent.fname != NULL)
        ret = strcmp(bb->ent.fname, aa->ent.fname);
    if (ret != 0)
        return ret;
    return bb->ent.fmtime - aa->ent.fmtime;
#else
    return ((CoverPvrSort *)b)->ent.fmtime - ((CoverPvrSort *)a)->ent.fmtime;
#endif
}

static int _pvr_file_sort(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    CoverPvrSort *sort = NULL;
    int i = 0;

    if(ctrl->paras.nents == 0)
        return 0;

    if((sort = GxCore_Calloc(ctrl->paras.nents, sizeof(CoverPvrSort))) == NULL)
    {
        COVER_PVR_ERR("malloc failed!\n");
        return -1;
    }
    for(i = 0; i < ctrl->paras.nents; i++)
    {
        sort[i].ent = ctrl->paras.ents[i];
        sort[i].attr = ctrl->paras.attrs[i];
        sort[i].info = ctrl->paras.infos[i];
    }
    qsort(sort, ctrl->paras.nents, sizeof(CoverPvrSort), _pvr_cmp);

    for(i = 0; i < ctrl->paras.nents; i++)
    {
        ctrl->paras.ents[i] = sort[i].ent;
        ctrl->paras.attrs[i] = sort[i].attr;
        ctrl->paras.infos[i] = sort[i].info;
    }
    GxCore_Free(sort);

    return 0;
}

static int _pvr_file_info_parse(int total)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    GxDirent *ent = NULL;
    FileAttr *attr = NULL;
    FileInfo *info = NULL;
    char *index_path = NULL;
    struct cover_pvr_epg_record *record = NULL;
    int record_num = 0;
    char **extra_path = NULL;
    int extra_num = 0;
    int record_cnt = 0;
    char *dir_path = NULL;
    char *info_path = NULL;
    int ret = -1;
    int i = 0, j = 0;
    bool not_find = true;
    int thread_id = 0;

    if((ctrl->mgr.first_in_flag & 1) && (extra_path = GxCore_Calloc(total, sizeof(char *))) == NULL)
        return -1;

    if((index_path = app_cover_pvr_get_full_name2(ctrl->path, PVR_INDEX_NAME)) == NULL)
        goto end;
    app_cover_pvr_parse_epg_record_all_info(index_path, &record, &record_num);

    if(0 == ctrl->filter.total)
    {
        for(i = 0; i < record_num; i++)
        {
            if((record) && (record[i].contents_num == 1) && record[i].contents)
            {
                if(strcmp(record[i].contents[0], STR_ID_FREE_SCAN_TAG) == 0)
                {
                    ctrl->filter.names[ctrl->filter.total] = GxCore_Strdup(STR_ID_FREE_SCAN_TAG);
                    ctrl->filter.total++;
                    break;
                }
            }
        }
    }

    for(i = 0; i < ctrl->paras.nents; i++)
    {
        ent = &ctrl->paras.ents[i];
        attr = &ctrl->paras.attrs[i];
        info = &ctrl->paras.infos[i];
        if(attr->flag == FILE_NODATA)
            continue;

        not_find = true;
        for(j = 0; j < record_num; j++)
        {
            if(record[j].fname && strcmp(ent->fname, record[j].fname) == 0)
            {
                not_find = false;
                _copy_record_to_file_info(&record[j], info);
                record[j].flag = EPG_RECORD_EXIST;
                record_cnt++;
                break;
            }
        }

        if(not_find && (ctrl->mgr.first_in_flag & 1))
        {
            dir_path = _get_dvr_dir_path_by_file(ent->fname);
            if((dir_path = _get_dvr_dir_path_by_file(ent->fname)) == NULL)
                goto end;
            if((info_path = app_cover_pvr_get_full_name2(dir_path, PVR_JSON_NAME)) == NULL)
            {
                APP_FREE(dir_path);
                goto end;
            }
            if(GxCore_FileExists(info_path) == GXCORE_FILE_EXIST)
            {
                extra_path[extra_num++] = info_path;
            }
            else
            {
                info->from_freescan = 1;
                APP_FREE(info_path);
            }
            APP_FREE(dir_path);
        }
    }
    _pvr_file_sort();

    if((record_cnt < record_num || extra_num > 0) && (ctrl->mgr.first_in_flag & 1))
    {
        ctrl->mgr.first_in_flag &= 2;
        GxCore_MutexLock(ctrl->mgr.mutex);
        ctrl->mgr.index_path = index_path;
        ctrl->mgr.record = record;
        ctrl->mgr.record_num = record_num;
        ctrl->mgr.extra_path = extra_path;
        ctrl->mgr.extra_num = extra_num;
        ctrl->mgr.thread_build_run = 1;
        GxCore_MutexUnlock(ctrl->mgr.mutex);
        GxCore_ThreadCreate("build_pvr_json", &thread_id, _cover_pvr_index_json_build, NULL, 1024 * 10, GXOS_DEFAULT_PRIORITY + 1);
        APP_TIMER_ADD(ctrl->rebuild_timer, _cover_pvr_rebuild_exec, 100, TIMER_REPEAT);
        return 0;
    }

    ret = 0;
end:
    APP_FREE(index_path);
    for(i = 0; i < extra_num; i++)
        APP_FREE(extra_path[i]);
    APP_FREE(extra_path);
    app_cover_pvr_free_epg_record_all(&record, &record_num);
    return ret;
}

static int _cover_pvr_rebuild_exec(void *userdata)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    const char *focus = NULL;
    bool flag = false;
    int i = 0;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if(ctrl->mgr.rebuild_flag == 1)
    {
        focus = GUI_GetFocusWindow();
        if(focus && strcmp(ctrl->wgt.wnd_name, focus) == 0)
        {
            ctrl->mgr.rebuild_flag = 0;
            flag = true;
        }
    }
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    if(flag)
    {
        for(i = 0; i < ctrl->paras.nents; i++)
            _file_info_free(&ctrl->paras.infos[i]);
        for(i = 0; i < ctrl->filter.total; i++)
            APP_FREE(ctrl->filter.names[i]);
        ctrl->filter.total = 0;

        _pvr_file_info_parse(ctrl->paras.total);
        if(ctrl->detail_flag == 1)
            _cover_pvr_details_hide();
        _cover_pvr_update_group_all(-1, 0, AREA_ITEM);
        COVER_PVR_INFO("\n");
        APP_TIMER_REMOVE(ctrl->rebuild_timer);
    }
    return 0;
}

#if TIMER_FLUSH_LOGO
static void _cover_pvr_copy_pvr_logo(void *arg)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int ret = -1;
    int i = 0;
    int choice = 0;
    char src[PVR_FULL_NAME_LEN];
    char dst[PVR_FULL_NAME_LEN];

    GxCore_ThreadDetach();
    while(1)
    {
        for(i = 0; i < PVR_ITEMS_PER_PAGE; i++)
        {
            GxCore_MutexLock(ctrl->mgr.mutex);
            if(ctrl->mgr.cache_stop_rq == 1)
                choice = 1;
            else if(ctrl->mgr.logo_stop_rq == 1)
                choice = 2;
            else if(ctrl->mgr.logo_copy_rq == 1)
            {
                if(ctrl->mgr.logo_para[i].state == LOGO_ADD && ctrl->mgr.logo_para[i].name != NULL)
                {
                    choice = 3;
                    ctrl->mgr.logo_para[i].state = LOGO_COPY;
                    snprintf(src, PVR_FULL_NAME_LEN, "%s", ctrl->mgr.logo_para[i].name);
                }
                else
                choice = 4;
            }
            else
                choice = 5;

            GxCore_MutexUnlock(ctrl->mgr.mutex);

            if(choice == 1)
                goto end;
            if(choice == 2)
                break;
            if(choice == 3)
            {
                snprintf(dst, PVR_FULL_NAME_LEN, CACHE_LOGO_COMMON, i + 1);
                ret = cover_pvr_copy_file_to_file(src, dst, 8192);
                GxCore_MutexLock(ctrl->mgr.mutex);
                if(ctrl->mgr.logo_para[i].state == LOGO_COPY)
                {
                    ctrl->mgr.logo_para[i].state = (ret > 0) ? LOGO_FIN : LOGO_IDLE;
                    if(i + 1 == PVR_ITEMS_PER_PAGE)
                        ctrl->mgr.logo_copy_rq = 0;
                }
                GxCore_MutexUnlock(ctrl->mgr.mutex);
            }
        }

        GxCore_MutexLock(ctrl->mgr.mutex);
        if(ctrl->mgr.logo_stop_rq == 1)
            ctrl->mgr.logo_stop_rq = 0;
        GxCore_MutexUnlock(ctrl->mgr.mutex);
        GxCore_ThreadDelay(10);
    }

end:
    GxCore_MutexLock(ctrl->mgr.mutex);
    for(i = 0; i < PVR_ITEMS_PER_PAGE; i++)
    {
        snprintf(dst, PVR_FULL_NAME_LEN, CACHE_LOGO_COMMON, i + 1);
        if(GxCore_FileExists(dst) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(dst);
    }
    ctrl->mgr.thread_cache_run = 0;
    ctrl->mgr.cache_stop_rq = 0;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}
#endif

static int _cover_pvr_systime_exec(void *usrdata)
{
#define TXT_SYS_TIME "text_cover_pvr_sys_time"
#define TXT_SYS_DATE "text_cover_pvr_sys_date"
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    const char *focus = GUI_GetFocusWindow();

    if(focus && strcmp(ctrl->wgt.wnd_name, focus) == 0)
        app_show_sys_time_and_date(ctrl->wgt.text_time, ctrl->wgt.text_date);
    return 0;
}

static int _pvr_file_paras_get(int sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    GxDirent *ent = &ctrl->paras.ents[sel];
    FileAttr *attr = &ctrl->paras.attrs[sel];
    DirPara dir_para;
    char *dir_path = NULL;
    int ret = -1;
    int i = 0;

    if(ent->fname && ent->ftype == GX_FILE_REGULAR)
    {
        dir_path = _get_dvr_dir_path_by_file(ent->fname);
        if(dir_path != NULL)
        {
            memset(&dir_para, 0, sizeof(dir_para));
            dir_para.nents = GxCore_GetDir(dir_path, &(dir_para.ents), PVR_MEDIE_SUFFIX);
            if(dir_para.nents > 0)
            {
                attr->size = 0;
                for(i = 0; i < dir_para.nents; i++)
                {
                    attr->size += dir_para.ents[i].fsize;
                }
                attr->time = dir_para.ents[i - 1].fmtime;
                ret = 0;
            }
            app_free_dir_ent(&dir_para.ents, &dir_para.nents);
        }
        APP_FREE(dir_path);
    }
    if(ret < 0)
        attr->flag |= FILE_NODATA;

    return ret;
}

int app_cover_pvr_ctrl_build(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int i = 0;
    int cnt = 0;

    if(ctrl->inited == true)
    {
        app_log_info("cover pvr ctrl already inited!\n");
        return -1;
    }

    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    if(pvr_partition_and_path_get(&ctrl->partition, ctrl->path, sizeof(ctrl->path)) == 0)
        return -1;

    // pvr medias
    ctrl->paras.nents = GxCore_GetDir(ctrl->path, &(ctrl->paras.ents), PVR_INDEX_SUFFIX);
    if(ctrl->paras.nents == 0)
    {
        app_free_dir_ent(&ctrl->paras.ents, &ctrl->paras.nents);
        APP_TIMER_ADD(ctrl->systime_timer, _cover_pvr_systime_exec, 800, TIMER_REPEAT);
        return 0;
    }
    GxCore_SortDir(ctrl->paras.ents, ctrl->paras.nents, NULL);

    ctrl->filter.names = GxCore_Calloc(PVR_CONTENT_TYPE_MAX, sizeof(char *));
    ctrl->uses = GxCore_Calloc(ctrl->paras.nents, sizeof(int));
    ctrl->paras.attrs = GxCore_Calloc(ctrl->paras.nents, sizeof(FileAttr));
    ctrl->paras.infos = GxCore_Calloc(ctrl->paras.nents, sizeof(FileInfo));
    if(!ctrl->filter.names || !ctrl->uses || !ctrl->paras.attrs || !ctrl->paras.infos)
    {
        COVER_PVR_ERR("malloc failed!\n");
        goto err;
    }
    for(i = 0; i < ctrl->paras.nents; i++)
    {
        if(_pvr_file_paras_get(i) == 0)
            cnt++;
    }

    if(cnt > 0)
        _pvr_file_info_parse(cnt);

    _cover_pvr_ctrl_filter();
#if TIMER_FLUSH_LOGO
    if(ctrl->mgr.first_in_flag & 2)
    {
        int thread_id = 0;
        ctrl->mgr.first_in_flag &= 1;
        GxCore_MutexLock(ctrl->mgr.mutex);
        ctrl->mgr.thread_cache_run = 1;
        GxCore_MutexUnlock(ctrl->mgr.mutex);
        GxCore_ThreadCreate("copy_pvr_logo", &thread_id, _cover_pvr_copy_pvr_logo, NULL, 1024 * 10, GXOS_DEFAULT_PRIORITY + 1);
    }
#endif
    APP_TIMER_ADD(ctrl->systime_timer, _cover_pvr_systime_exec, 800, TIMER_REPEAT);
    ctrl->inited = true;

    return 0;
err:
    _cover_pvr_ctrl_release();
    return -1;
}

static int _cover_pvr_ctrl_filter_pop(AreaSel area_sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    int sel = 0;
    int i = 0;
    int total = 0;
    char **genre_name_array = NULL;
    PopList pop_list;

    total = ctrl->filter.total + 1;
    if((genre_name_array = GxCore_Calloc(total, sizeof(char *))) == NULL)
    {
        COVER_PVR_ERR("malloc failed!");
        return 0;
    }
    genre_name_array[0] = STR_ID_ALL;
    for(i = 1; i < total; i++)
    {
        genre_name_array[i] = ctrl->filter.names[i - 1];
    }

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.sel = ctrl->filter_sel;
    pop_list.show_num = false;
    pop_list.title = STR_ID_RECORD_FILTER;
    pop_list.item_num = total;
    pop_list.item_content = genre_name_array;
    sel = poplist_create(&pop_list);

    if((sel < total && sel >= 0) && sel != ctrl->filter_sel)
    {
        GUI_SetProperty(ctrl->wgt.text_item_filter, "string", genre_name_array[sel]);
        _cover_pvr_update_group_all(-1, sel, area_sel);
    }

    APP_FREE(genre_name_array);
    return sel;
}

static char *_cover_pvr_duration_str_get(time_t start_time, time_t finish_time)
{
    static char buffer[32];
    char *start_str = NULL;
    char *finish_str = NULL;
    char *date_str = NULL;
    char *join_str = NULL;
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    memset(buffer, 0, sizeof(buffer));
    join_str = " - ";
    start_str = app_time_to_hourmin_edit_str(start_time);
    finish_str = app_time_to_hourmin_edit_str(finish_time);
    if(!start_str && !finish_str)
        goto end;

    date_str = app_time_to_date_edit_str(start_time);
    if(!date_str)
        goto end;
    if(ctrl->wgt.reverse_flag)
        snprintf(buffer, sizeof(buffer), "%s%s%s %s", finish_str, join_str, start_str, date_str);
    else
        snprintf(buffer, sizeof(buffer), "%s %s%s%s", date_str, start_str, join_str, finish_str);

end:
    APP_FREE(start_str);
    APP_FREE(finish_str);
    APP_FREE(date_str);
    return buffer;
}

static char *duration_str_get(time_t start, time_t finish)
{
    time_t start_sec = 0, finish_sec = 0;
    start_sec = get_display_time_by_timezone(start);
    finish_sec = get_display_time_by_timezone(finish);
    return _cover_pvr_duration_str_get(start_sec, finish_sec);
}

static void clear_cover_pvr_info(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    app_set_widget_string(ctrl->wgt.text_item_duration, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_name, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_parent, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_genres, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_brief, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_ch_name, STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", STR_ID_BLANK);

    app_set_widget_string(ctrl->wgt.text_item_size, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_dir, STR_ID_BLANK);
    app_set_widget_string(ctrl->wgt.text_item_file, STR_ID_NO_PVR_FILE);
}

static void display_cover_pvr_info(void)
{
#define _BUFFER_LEN 256
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    FileAttr *attr = &ctrl->paras.attrs[ctrl->uses[ctrl->cur_index]];
    FileInfo *info = &ctrl->paras.infos[ctrl->uses[ctrl->cur_index]];
    char *tmp_str = NULL;
    char *dir_path = NULL, *logo_path = NULL;
    char rate_buf[8] = { 0 };
    int i = 0;
    char buffer[_BUFFER_LEN] = { 0 };

    if(info->start && info->finish)
    {
        tmp_str = duration_str_get(info->start, info->finish);
        app_set_widget_string(ctrl->wgt.text_item_duration, tmp_str);
    }
    else if(info->recstart && info->duration)
    {
        tmp_str = duration_str_get(info->recstart, info->recstart + info->duration);
        app_set_widget_string(ctrl->wgt.text_item_duration, tmp_str);
    }
    else
    {
        app_set_widget_string(ctrl->wgt.text_item_duration, STR_ID_BLANK);
    }

    if(info->title)
    {
        app_set_widget_string(ctrl->wgt.text_item_name, info->title);
    }
    else
    {
        app_set_widget_string(ctrl->wgt.text_item_name, STR_ID_BLANK);
    }

    if(info->rate)
    {
        snprintf(rate_buf, sizeof(rate_buf), "%d", info->rate);
        app_set_widget_string(ctrl->wgt.text_item_parent, rate_buf);
    }
    else
    {
        app_set_widget_string(ctrl->wgt.text_item_parent, STR_ID_BLANK);
    }

    if(info->nfilters == 0)
    {
        app_set_widget_string(ctrl->wgt.text_item_genres, STR_ID_BLANK);
    }
    else
    {
        memset(buffer, 0, _BUFFER_LEN);
        for(i = 0; i < info->nfilters; i++)
        {
            if(strlen(buffer) + strlen(ctrl->filter.names[info->filters[i]]) < _BUFFER_LEN)
            {
                strcat(buffer, ctrl->filter.names[info->filters[i]]);
                if(i < info->nfilters - 1)
                {
                    if(strlen(buffer) + strlen(" / ") < _BUFFER_LEN)
                        strcat(buffer, " / ");
                    else
                        break;
                }
            }
        }
        app_set_widget_string(ctrl->wgt.text_item_genres, buffer);
    }

    tmp_str = info->desc;
    app_set_widget_string(ctrl->wgt.text_item_brief, tmp_str);

    memset(buffer, 0, _BUFFER_LEN);
    app_display_size_str_get(attr->size, buffer, _BUFFER_LEN);
    app_set_widget_string(ctrl->wgt.text_item_size, buffer);

    memset(buffer, 0, _BUFFER_LEN);
    snprintf(buffer, _BUFFER_LEN, "%s %s", ctrl->partition.partition_name, PVR_DIR_STR);
    app_set_widget_string(ctrl->wgt.text_item_dir, buffer);

    app_set_widget_string(ctrl->wgt.text_item_file, ctrl->paras.ents[ctrl->uses[ctrl->cur_index]].fname);

    // channel name and logo
    app_set_widget_string(ctrl->wgt.text_ch_name, info->chname);
    if(ctrl->wgt.chlogo_bg_flag)
    {
        ctrl->wgt.chlogo_bg_flag = false;
        GUI_SetProperty(ctrl->wgt.text_ch_logo, "backcolor", IDLE_BG2_COLOR);
    }
    else
    {
        ctrl->wgt.chlogo_bg_flag = true;
        GUI_SetProperty(ctrl->wgt.text_ch_logo, "backcolor", IDLE_BG_COLOR);
    }
    if((dir_path = _get_dvr_dir_path_by_index(ctrl->cur_index)) != NULL)
        logo_path = app_cover_pvr_get_full_name2(dir_path, PVR_CHLOGO_NAME);
    if(logo_path && GxCore_FileExists(logo_path) == GXCORE_FILE_EXIST)
    {
        gal_add_key_path(RECORD_CHKEY, logo_path);
        GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", RECORD_CHKEY);
    }
    else
    {
        GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", STR_ID_BLANK);
    }
    APP_FREE(dir_path);
    APP_FREE(logo_path);
}

static void _cover_pvr_info_update(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    if(ctrl->nuses == 0 || ctrl->uses == NULL || ctrl->cur_index < 0 || ctrl->cur_index >= ctrl->nuses)
        clear_cover_pvr_info();
    else
        display_cover_pvr_info();
}

static void _cover_pvr_change_focus_sel(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };
    int old_sel = 0;
    int new_sel = 0;

    // cal new_focus_sel
    old_sel = (ctrl->wgt.reverse_flag) ? ctrl->rfocus_sel : ctrl->focus_sel;
    switch(ctrl->area_sel)
    {
        case AREA_ITEM:
            if(ctrl->nuses == 0)
                new_sel = -1;
            else
                new_sel = ctrl->cur_index % PVR_ITEMS_PER_PAGE;
            break;
        case AREA_GROUP:
            new_sel = ctrl->filter_sel % PVR_GROUPS_PER_PAGE + PVR_ITEMS_PER_PAGE;
            break;
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            new_sel = PVR_ITEMS_PER_PAGE + PVR_GROUPS_PER_PAGE;
            break;
#endif
        default:
            break;
    }
    (ctrl->wgt.reverse_flag) ? (ctrl->rfocus_sel = new_sel) : (ctrl->focus_sel = new_sel);
    if(old_sel == new_sel)
    {
#if PVR_PREVIEW_SUPPORT
        if (ctrl->area_sel == AREA_ITEM && ctrl->nuses > 0) {
            play_movie_stop();
            APP_TIMER_ADD(ctrl->preview_timer, _cover_pvr_preview, 1000, TIMER_REPEAT);
            APP_TIMER_ADD(ctrl->preview_stop_timer, _cover_pvr_preview_stop, 30000, TIMER_REPEAT);
        }
#endif
        return;
    }

    // set unfocus img
    if(old_sel < 0)
    {
        // do nothing
    }
    else if(old_sel < PVR_ITEMS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, old_sel + 1);
        GUI_SetProperty(wgt, "img", ITEM_UNFOCUS_IMG);
#if PVR_PREVIEW_SUPPORT
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_preview_common, old_sel + 1);
        GUI_SetProperty(wgt, "state", "hide");
#endif
    }
    else if(old_sel < PVR_ITEMS_PER_PAGE + PVR_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, old_sel - PVR_ITEMS_PER_PAGE + 1);
        GUI_SetProperty(wgt, "focus_img", GROUP_FOCUS_IMG);
    }
    else
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, PVR_GROUPS_PER_PAGE + 1);
        GUI_SetProperty(wgt, "focus_img", GROUP_FOCUS_IMG);
    }

    // set focus img
    if(new_sel < 0)
    {
        // do nothing
    }
    else if(new_sel < PVR_ITEMS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, new_sel + 1);
        GUI_SetProperty(wgt, "img", ITEM_FOCUS_IMG);
    }
    else if(new_sel < PVR_ITEMS_PER_PAGE + PVR_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, new_sel - PVR_ITEMS_PER_PAGE + 1);
        GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
    }
    else
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, PVR_GROUPS_PER_PAGE + 1);
        GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
    }

#if PVR_PREVIEW_SUPPORT
    if (ctrl->area_sel == AREA_ITEM && ctrl->nuses > 0) {
        play_movie_stop();
        APP_TIMER_ADD(ctrl->preview_timer, _cover_pvr_preview, 1000, TIMER_REPEAT);
        APP_TIMER_ADD(ctrl->preview_stop_timer, _cover_pvr_preview_stop, 30000, TIMER_REPEAT);
    }else {
        APP_TIMER_REMOVE(ctrl->preview_timer);
        _cover_pvr_preview_stop(NULL);
    }
#endif

    // set focus btn
#if MORE_GROUP_SUPPORT
    if(ctrl->area_sel == AREA_MORE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, PVR_GROUPS_PER_PAGE + 1);
        GUI_SetFocusWidget(wgt);
    }
    else
#endif
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, ctrl->filter_sel % PVR_GROUPS_PER_PAGE + 1);
        GUI_SetFocusWidget(wgt);
    }
}
static void _cover_pvr_change_ok_tip(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    switch(ctrl->area_sel)
    {
        case AREA_ITEM:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", "s_ts_ok");
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_PLAY);
            break;
        case AREA_GROUP:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", STR_ID_BLANK);
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_BLANK);
            break;
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", "s_ts_ok");
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_MORE);
            break;
#endif
        default:
            break;
    }
}

static void _cover_pvr_change_area_sel(AreaSel new_sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    ctrl->area_sel = new_sel;
    _cover_pvr_change_focus_sel();
#if MORE_GROUP_SUPPORT
    if(new_sel == AREA_MORE)
        _cover_pvr_ctrl_filter_pop(AREA_GROUP);
#endif
    _cover_pvr_change_ok_tip();
}

static void _cover_pvr_hide_one_item(int i)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, i + 1);
    GUI_SetProperty(wgt, "img", ITEM_UNFOCUS_IMG);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i + 1);
    GUI_SetProperty(wgt, "img", STR_ID_BLANK);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, i + 1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_time_common, i + 1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, i + 1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
#if PVR_PREVIEW_SUPPORT
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_preview_common, i + 1);
    GUI_SetProperty(wgt, "state", "hide");
#endif
}

static void _cover_pvr_show_one_item(int i, int first_sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    FileInfo *info = NULL;
    char *tmp_str = NULL;
    char tmp_buf[32] = { 0 };
    int sel = 0, csec = 0;
    char *dir_path = NULL, *logo_path = NULL;
    char wgt[ITEM_COMMON_SIZE] = { 0 };

    sel = first_sel + i;
    info = &ctrl->paras.infos[ctrl->uses[sel]];

#if PVR_PREVIEW_SUPPORT
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_preview_common, i + 1);
    GUI_SetProperty(wgt, "state", "hide");
#endif
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, i + 1);
    tmp_str = info->title;
    if(!tmp_str || strlen(tmp_str) == 0)
    {
        if(info->recstart && info->duration)
        {
            tmp_str = duration_str_get(info->recstart, info->recstart + info->duration);
        }
        else
        {
            tmp_str = ctrl->paras.ents[ctrl->uses[sel]].fname;
        }
    }
    app_set_widget_string(wgt, tmp_str);

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_time_common, i + 1);
    if(info->duration)
    {
        csec = (int)info->duration;
        snprintf(tmp_buf, sizeof(tmp_buf), "%02dh%02dm%02ds", csec / 3600, csec % 3600 / 60, csec % 3600 % 60);
        tmp_str = tmp_buf;
    }
    else
    {
        tmp_str = STR_ID_BLANK;
    }
    app_set_widget_string(wgt, tmp_str);

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, i + 1);
    app_set_widget_string(wgt, info->chname);

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i + 1);
    if((dir_path = _get_dvr_dir_path_by_index(sel)) != NULL)
        logo_path = app_cover_pvr_get_full_name2(dir_path, PVR_LOGO_NAME);
    if(logo_path && GxCore_FileExists(logo_path) == GXCORE_FILE_EXIST)
    {
#if TIMER_FLUSH_LOGO
        GUI_SetProperty(wgt, "img",        COVER_PVR_EVENT_KEY_DF);
        GxCore_MutexLock(ctrl->mgr.mutex);
        ctrl->mgr.logo_para[i].state = LOGO_ADD;
        ctrl->mgr.logo_para[i].name = logo_path;
        logo_path = NULL;
        GxCore_MutexUnlock(ctrl->mgr.mutex);
#else
        char key[ITEM_COMMON_SIZE] = { 0 };
        snprintf(key, ITEM_COMMON_SIZE, ITEM_KEY_COMMON, i + 1);
        gal_add_key_path(key, logo_path);
        GUI_SetProperty(wgt, "img", key);
#endif
    }
    else
    {
        GUI_SetProperty(wgt, "img", COVER_PVR_EVENT_KEY_DF);
    }
    APP_FREE(dir_path);
    APP_FREE(logo_path);
}

#if TIMER_FLUSH_LOGO
static void _cover_pvr_clean_logo_data(bool set_flag)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int i = 0;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if(set_flag)
    {
        ctrl->mgr.logo_stop_rq = 1;
        ctrl->mgr.logo_copy_rq = 0;
    }
    for(i = 0; i < PVR_ITEMS_PER_PAGE; i++)
    {
        APP_FREE(ctrl->mgr.logo_para[i].name);
    }
    memset(ctrl->mgr.logo_para, 0, sizeof(ctrl->mgr.logo_para));
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}

static int _cover_pvr_flush_logo_timer_exec(void *userdata)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    const char *focus = NULL;

    if(ctrl->detail_flag == 0)
    {
        focus = GUI_GetFocusWindow();
        if(focus && strcmp(ctrl->wgt.wnd_name, focus) == 0)
        {
            bool flag = false;
            int i, new_sel, first_sel, show_num;
            char key[ITEM_COMMON_SIZE] = { 0 };
            char wgt[ITEM_COMMON_SIZE] = { 0 };
            char logo[PVR_FULL_NAME_LEN] = { 0 };

            new_sel = ctrl->cur_index;
            first_sel = new_sel - new_sel % PVR_ITEMS_PER_PAGE;
            show_num = ctrl->nuses - first_sel;
            if(show_num > PVR_ITEMS_PER_PAGE)
                show_num = PVR_ITEMS_PER_PAGE;
            for(i = 0; i < show_num; i++)
            {
                flag = false;
                GxCore_MutexLock(ctrl->mgr.mutex);
                if(ctrl->mgr.logo_para[i].state == LOGO_FIN)
                {
                    flag = true;
                    ctrl->mgr.logo_para[i].state = LOGO_IDLE;
                }
                GxCore_MutexUnlock(ctrl->mgr.mutex);

                if(flag)
                {
                    snprintf(key, ITEM_COMMON_SIZE, ITEM_KEY_COMMON, i + 1);
                    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i + 1);
                    snprintf(logo, PVR_FULL_NAME_LEN, CACHE_LOGO_COMMON, i + 1);
                    if(GxCore_FileExists(logo) == GXCORE_FILE_EXIST)
                    {
                        gal_add_key_path(key, logo);
                        GUI_SetProperty(wgt, "img", key);
                    }
                }
            }

            flag = true;
            GxCore_MutexLock(ctrl->mgr.mutex);
            for(i = 0; i < PVR_ITEMS_PER_PAGE; i++)
            {
                if(ctrl->mgr.logo_para[i].state != LOGO_IDLE)
                {
                    flag = false;
                    break;
                }
            }
            GxCore_MutexUnlock(ctrl->mgr.mutex);

            if(flag)
            {
                APP_TIMER_REMOVE(ctrl->logo_timer);
            }
        }
    }

    return 0;
}
#endif

static int _cover_pvr_update_item_all(int last_sel, int new_sel, bool focus_change)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int first_sel, show_num;
    int last_page, new_page, total_page;
    char buffer[32] = { 0 };
    int i;

    ctrl->cur_index = new_sel;
    if(focus_change)
        _cover_pvr_change_focus_sel();

    if(ctrl->nuses == 0 || new_sel < 0)
    {
#if TIMER_FLUSH_LOGO
        _cover_pvr_clean_logo_data(true);
#endif
        for(i = 0; i < PVR_ITEMS_PER_PAGE; i++)
        {
            _cover_pvr_hide_one_item(i);
        }
        app_set_widget_string(ctrl->wgt.text_item_page, "0/0");
    }
    else
    {
        last_page = last_sel / PVR_ITEMS_PER_PAGE + 1;
        new_page = new_sel / PVR_ITEMS_PER_PAGE + 1;
        if(last_sel < 0 || last_page != new_page)
        {
#if TIMER_FLUSH_LOGO
            _cover_pvr_clean_logo_data(true);
#endif
            first_sel = new_sel - new_sel % PVR_ITEMS_PER_PAGE;
            show_num = ctrl->nuses - first_sel;
            if(show_num > PVR_ITEMS_PER_PAGE)
                show_num = PVR_ITEMS_PER_PAGE;
            for(i = 0; i < show_num; i++)
            {
                _cover_pvr_show_one_item(i, first_sel);
            }
            for(i = show_num; i < PVR_ITEMS_PER_PAGE; i++)
            {
                _cover_pvr_hide_one_item(i);
            }
#if TIMER_FLUSH_LOGO
            GxCore_MutexLock(ctrl->mgr.mutex);
            ctrl->mgr.logo_stop_rq = 1;
            ctrl->mgr.logo_copy_rq = 1;
            GxCore_MutexUnlock(ctrl->mgr.mutex);
            APP_TIMER_ADD(ctrl->logo_timer, _cover_pvr_flush_logo_timer_exec, 20, TIMER_REPEAT);
#endif
        }

        total_page = (ctrl->nuses - 1) / PVR_ITEMS_PER_PAGE + 1;
        snprintf(buffer, sizeof(buffer), "%d/%d", new_page, total_page);
        app_set_widget_string(ctrl->wgt.text_item_page, buffer);
    }

    _cover_pvr_info_update();

    return 0;
}

static void _cover_pvr_hide_one_group(int i)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, i + 1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
}

static void _cover_pvr_show_one_group(int i, int first_sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };
    int sel = 0;

    sel = first_sel + i;
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, i + 1);
    if(sel == 0)
        GUI_SetProperty(wgt, "string", STR_ID_ALL);
    else
        GUI_SetProperty(wgt, "string", ctrl->filter.names[sel - 1]);

    if(sel == ctrl->filter_sel)
    {
#if MORE_GROUP_SUPPORT
        if(ctrl->area_sel != AREA_MORE)
#endif
        GUI_SetFocusWidget(wgt);
    }
}

static int _cover_pvr_update_group_all(int last_sel, int new_sel, AreaSel area_sel)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int first_sel, show_num;
    int last_page, new_page;
    int i;

    if(new_sel > ctrl->filter.total)
        new_sel = 0;
    if(last_sel == new_sel)
        return 0;

    last_page = last_sel / PVR_GROUPS_PER_PAGE + 1;
    new_page = new_sel / PVR_GROUPS_PER_PAGE + 1;
    ctrl->area_sel = area_sel;
    ctrl->filter_sel = new_sel;
    if(last_sel < 0 || last_sel != new_sel)
    {
        if(new_sel == 0)
            GUI_SetProperty(ctrl->wgt.text_item_filter, "string", STR_ID_ALL);
        else
            GUI_SetProperty(ctrl->wgt.text_item_filter, "string", ctrl->filter.names[new_sel - 1]);
        _cover_pvr_ctrl_filter();
        _cover_pvr_update_item_all(-1, 0, false);
        _cover_pvr_change_focus_sel();
    }

    if(last_sel < 0 || last_page != new_page)
    {
        first_sel = new_sel - new_sel % PVR_GROUPS_PER_PAGE;
        show_num = ctrl->filter.total + 1 - first_sel;
        if(show_num > PVR_GROUPS_PER_PAGE)
            show_num = PVR_GROUPS_PER_PAGE;
        for(i = 0; i < show_num; i++)
        {
            _cover_pvr_show_one_group(i, first_sel);
        }
        for(i = show_num; i < PVR_GROUPS_PER_PAGE; i++)
        {
            _cover_pvr_hide_one_group(i);
        }
    }

    return 0;
}

#if PVR_PREVIEW_SUPPORT
static int _cover_pvr_preview(void *usrdata)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    PlayerWindow window;
    char wgt[ITEM_COMMON_SIZE] = { 0 };
    char rect_str[50] = {0};
    char *url = NULL;
    size_t url_len = 0;
    GxDirent *dir = &ctrl->paras.ents[ctrl->uses[ctrl->cur_index]];
    FileInfo *info = &ctrl->paras.infos[ctrl->uses[ctrl->cur_index]];

    APP_TIMER_REMOVE(ctrl->preview_timer);
    if (ctrl->area_sel != AREA_ITEM) {
        return 0;
    }

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_preview_common, ctrl->focus_sel + 1);
    GUI_SetProperty(wgt, "state", "show");
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, ctrl->focus_sel + 1);
    if (GXCORE_SUCCESS != GUI_GetProperty(wgt, "rect", rect_str))
        return -1;

	sscanf(rect_str, "[%d,%d,%d,%d]",&window.x, &window.y, &window.width, &window.height);

    url_len = strlen(ctrl->path) + strlen(dir->fname) + 2; // +2 for '/' and '\0'
    url = GxCore_Malloc(url_len);
    if(url == NULL)
    {
        app_log_error("malloc failed!\n");
        return -1;
    }
    snprintf(url, url_len, "%s/%s", ctrl->path, dir->fname);
    if (info->preview_flag == PREVIEW_FROM_TIME) {
        play_movie_zoom_by_path(url, &window, info->preview_offset * 1000);
    }
    else if (info->preview_flag == PREVIEW_FROM_PATH && info->preview_path) {
        play_movie_zoom_by_path(info->preview_path, &window, 0);
    }
    else {
        play_movie_zoom_by_path(url, &window, 0);
    }
    GUI_SetInterface("video_on", NULL);
    GxCore_Free(url);

    return 0;
}

static int _cover_pvr_preview_stop(void *usrdata)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };

    APP_TIMER_REMOVE(ctrl->preview_stop_timer);
    play_movie_stop();
    if (ctrl->area_sel == AREA_ITEM) {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_preview_common, ctrl->focus_sel + 1);
        GUI_SetProperty(wgt, "state", "hide");
    }

    return 0;
}
#endif

static int _cover_pvr_change_item_keypress(uint16_t key)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int total;
    int total_page, last_page;
    int last_sel, new_sel, item_sel, first_sel;

    if(ctrl->nuses == 0)
    {
        if(key == STBK_UP || key == STBK_DOWN)
            _cover_pvr_change_area_sel(AREA_GROUP);
        return EVENT_TRANSFER_STOP;
    }

    total = ctrl->nuses;
    total_page = (total - 1) / PVR_ITEMS_PER_PAGE + 1;

    last_sel = ctrl->cur_index;
    item_sel = last_sel % PVR_ITEMS_PER_PAGE;
    first_sel = last_sel - item_sel;
    last_page = last_sel / PVR_ITEMS_PER_PAGE + 1;

    switch(key)
    {
        case STBK_PAGE_UP:
            if(last_page == 1)
            {
                new_sel = (total_page - 1) * PVR_ITEMS_PER_PAGE + item_sel;
                if(new_sel >= total)
                    new_sel = total - 1;
            }
            else
            {
                new_sel = last_sel - PVR_ITEMS_PER_PAGE;
            }
            break;
        case STBK_PAGE_DOWN:
            if(last_page == total_page)
            {
                new_sel = item_sel;
            }
            else
            {
                new_sel = last_sel + PVR_ITEMS_PER_PAGE;
                if(new_sel >= total)
                    new_sel = total - 1;
            }
            break;
        case STBK_UP:
            if(last_sel - first_sel < PVR_ITEMS_ROW_NUMS)
            {
                _cover_pvr_change_area_sel(AREA_GROUP);
                return EVENT_TRANSFER_STOP;
            }
            else
            {
                new_sel = last_sel - PVR_ITEMS_ROW_NUMS;
            }
            break;
        case STBK_DOWN:
            if(last_sel - first_sel >= PVR_ITEMS_PER_PAGE - PVR_ITEMS_ROW_NUMS)
            {
                _cover_pvr_change_area_sel(AREA_GROUP);
                return EVENT_TRANSFER_STOP;
            }
            else
            {
                new_sel = last_sel + PVR_ITEMS_ROW_NUMS;
                if(new_sel >= total)
                    new_sel = total - 1;
            }
            break;
#if LEFT_RIGHT_SERIAL_MODE
        case STBK_LEFT:
            new_sel = last_sel - 1;
            if(new_sel < 0)
                new_sel = total - 1;
            break;
        case STBK_RIGHT:
            new_sel = last_sel + 1;
            if(new_sel >= total)
                new_sel = 0;
            break;
#else
        case STBK_LEFT:
            if(last_sel % PVR_ITEMS_ROW_NUMS == 0)
            {
                if(last_page == 1)
                {
                    new_sel = last_sel + (total_page - 1) * PVR_ITEMS_PER_PAGE + PVR_ITEMS_ROW_NUMS - 1;
                }
                else
                {
                    new_sel = last_sel - PVR_ITEMS_PER_PAGE + PVR_ITEMS_ROW_NUMS - 1;
                }
                if(new_sel >= total)
                    new_sel = total - 1;
            }
            else
            {
                new_sel = last_sel - 1;
            }
            break;
        case STBK_RIGHT:
            if(last_sel == total - 1)
            {
                new_sel = item_sel - item_sel % PVR_ITEMS_ROW_NUMS;
            }
            else if((last_sel + 1) % PVR_ITEMS_ROW_NUMS == 0)
            {
                if(last_page == total_page)
                {
                    new_sel = item_sel - item_sel % PVR_ITEMS_ROW_NUMS;
                }
                else
                {
                    new_sel = last_sel + PVR_ITEMS_PER_PAGE - item_sel % PVR_ITEMS_ROW_NUMS;
                }
            }
            else
            {
                new_sel = last_sel + 1;
            }

            if(new_sel >= total)
                new_sel = total - 1;
            break;
#endif
        default:
            return EVENT_TRANSFER_STOP;
    }

    _cover_pvr_update_item_all(last_sel, new_sel, true);

    return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_change_group_keypress(uint16_t key)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int filter_sel = ctrl->filter_sel;
#if MORE_GROUP_SUPPORT
    int item_sel = 0;
#endif

    switch(key)
    {
        case STBK_UP:
        case STBK_DOWN:
            _cover_pvr_change_area_sel(AREA_ITEM);
            break;
#if MORE_GROUP_SUPPORT
        case STBK_LEFT:
            item_sel = filter_sel % PVR_GROUPS_PER_PAGE;
            if(item_sel == 0)
                _cover_pvr_change_area_sel(AREA_MORE);
            else
                _cover_pvr_update_group_all(filter_sel, filter_sel - 1, AREA_GROUP);
            break;
        case STBK_RIGHT:
            item_sel = filter_sel % PVR_GROUPS_PER_PAGE;
            if(item_sel == PVR_GROUPS_PER_PAGE - 1 || filter_sel == ctrl->filter.total)
                _cover_pvr_change_area_sel(AREA_MORE);
            else
                _cover_pvr_update_group_all(filter_sel, filter_sel + 1, AREA_GROUP);
            break;
#else
        case STBK_LEFT:
            if(filter_sel == 0)
                _cover_pvr_update_group_all(filter_sel, ctrl->filter.total, AREA_GROUP);
            else
                _cover_pvr_update_group_all(filter_sel, filter_sel - 1, AREA_GROUP);
            break;
        case STBK_RIGHT:
            if(filter_sel == ctrl->filter.total)
                _cover_pvr_update_group_all(filter_sel, 0, AREA_GROUP);
            else
                _cover_pvr_update_group_all(filter_sel, filter_sel + 1, AREA_GROUP);
            break;
#endif
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

#if MORE_GROUP_SUPPORT
static int _cover_pvr_change_more_keypress(uint16_t key)
{
    switch(key)
    {
        case STBK_UP:
        case STBK_DOWN:
            _cover_pvr_change_area_sel(AREA_ITEM);
            break;
        case STBK_LEFT:
        case STBK_RIGHT:
            _cover_pvr_change_area_sel(AREA_GROUP);
            break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}
#endif

static int _cover_pvr_change_keypress(uint16_t key)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int ret = EVENT_TRANSFER_STOP;

    if(ctrl->wgt.reverse_flag)
    {
        if(key == STBK_LEFT)
            key = STBK_RIGHT;
        else if(key == STBK_RIGHT)
            key = STBK_LEFT;
    }

    switch(ctrl->area_sel)
    {
        case AREA_ITEM:
            _cover_pvr_change_item_keypress(key);
            break;
        case AREA_GROUP:
            _cover_pvr_change_group_keypress(key);
            break;
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            app_cover_pvr_change_more_keypress(key);
            break;
#endif
        default:
            break;
    }
    return ret;
}

static int _cover_pvr_details_keypress(uint16_t key, const char *widget)
{
    int value = 1;

    switch(key)
    {
        case STBK_UP:
            GUI_SetProperty(widget, "line_up", &value);
            break;
        case STBK_DOWN:
            GUI_SetProperty(widget, "line_down", &value);
            break;
        case STBK_PAGE_UP:
            GUI_SetProperty(widget, "page_up", &value);
            break;
        case STBK_PAGE_DOWN:
            GUI_SetProperty(widget, "page_down", &value);
            break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

static void _cover_pvr_details_hide(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    ctrl->detail_flag = 0;
    GUI_SetProperty(ctrl->wgt.details_duration, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_title, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_notepad, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_duration, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_title, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_notepad, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_exit, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_back, "state", "hide");
}

static void _cover_pvr_show_no_info_popdlg(void)
{
    PopDlg pop = { 0 };
    pop.type = POP_TYPE_OK;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = STR_ID_NO_INFO;
    pop.timeout_sec = 3;
    popdlg_create(&pop);
}

static void _cover_pvr_details_show(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    FileInfo *info = NULL;
    char *tmp_str = NULL;

    if(ctrl->nuses == 0)
        return;
    info = &ctrl->paras.infos[ctrl->uses[ctrl->cur_index]];
    if(info->start == 0)
    {
        _cover_pvr_show_no_info_popdlg();
        return;
    }

    ctrl->detail_flag = 1;
    GUI_SetProperty(ctrl->wgt.details_back, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_duration, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_title, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_notepad, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_exit, "state", "show");

    tmp_str = duration_str_get(info->start, info->finish);
    GUI_SetProperty(ctrl->wgt.details_duration, "string", tmp_str);
    tmp_str = info->desc;
    GUI_SetProperty(ctrl->wgt.details_title, "string", info->title);
    GUI_SetProperty(ctrl->wgt.details_notepad, "string", tmp_str);
}

int app_cover_pvr_event_info_free(play_record_info *rec_info)
{
    APP_FREE(rec_info->event_logo);
    APP_FREE(rec_info->genres_str);
    memset(rec_info, 0, sizeof(play_record_info));
    return 0;
}

int app_cover_pvr_event_info_get(int num, const char *fname, bool get_all, play_record_info *rec_info)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    FileInfo *info = NULL;
    int i = 0;
    int cur_sel = -1;

    if(num >= ctrl->nuses)
        return -1;

    if(strcmp(ctrl->paras.ents[ctrl->uses[num]].fname, fname) == 0)
    {
        cur_sel = num;
    }
    if(cur_sel == -1)
    {
        for(i = 0; i < ctrl->nuses; i++)
        {
            if(strcmp(ctrl->paras.ents[ctrl->uses[i]].fname, fname) == 0)
            {
                cur_sel = i;
                break;
            }
        }
        if(cur_sel == -1)
            return -1;
    }

    info = &ctrl->paras.infos[ctrl->uses[cur_sel]];
    rec_info->event_title = info->title;
    if(!get_all)
        return 0;

    rec_info->start_time = info->start;
    rec_info->finish_time = info->finish;
    rec_info->parent_rate = info->rate;
    rec_info->event_desc = info->desc;

    // genres_str
    if(info->nfilters == 0)
    {
        rec_info->genres_str = NULL;
    }
    else
    {
#define FILTER_BUF_LEN 256
        char *buffer = GxCore_Calloc(FILTER_BUF_LEN, 1);
        if(buffer)
        {
            for(i = 0; i < info->nfilters; i++)
            {
                if(strlen(buffer) + strlen(ctrl->filter.names[info->filters[i]]) < FILTER_BUF_LEN)
                {
                    strcat(buffer, ctrl->filter.names[info->filters[i]]);
                    if(i < info->nfilters - 1)
                    {
                        if(strlen(buffer) + strlen(" / ") < FILTER_BUF_LEN)
                            strcat(buffer, " / ");
                        else
                            break;
                    }
                }
            }
        }
        rec_info->genres_str = buffer;
    }

    // event_logo
    char *dir_path = NULL, *logo_path = NULL;
    if((dir_path = _get_dvr_dir_path_by_index(cur_sel)) != NULL)
        logo_path = app_cover_pvr_get_full_name2(dir_path, PVR_LOGO_NAME);
    APP_FREE(dir_path);
    if(logo_path && GxCore_FileExists(logo_path) == GXCORE_FILE_EXIST)
    {
        rec_info->event_logo = logo_path;
    }
    else
    {
        APP_FREE(logo_path);
        rec_info->event_logo = NULL;
    }

    return 0;
}

int app_cover_pvr_event_name_and_logo_get(int num, const char *fname, char **title, char **chlogo, char *ptime, int size)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    FileInfo *info = NULL;
    int i = 0;
    int cur_sel = -1;

    if(num >= ctrl->nuses)
        return -1;
    if(!title && !chlogo)
        return -1;

    if(strcmp(ctrl->paras.ents[ctrl->uses[num]].fname, fname) == 0)
    {
        cur_sel = num;
    }
    if(cur_sel == -1)
    {
        for(i = 0; i < ctrl->nuses; i++)
        {
            if(strcmp(ctrl->paras.ents[ctrl->uses[i]].fname, fname) == 0)
            {
                cur_sel = i;
                break;
            }
        }
        if(cur_sel == -1)
            return -1;
    }

    info = &ctrl->paras.infos[ctrl->uses[cur_sel]];
    if(title)
    {
        char *tmp_title = NULL;
        if(info->title)
        {
            tmp_title = info->title;
        }
        else if(info->chname)
        {
            tmp_title = info->chname;
        }

        if(tmp_title)
        {
            if(info->recstart && info->duration)
            {
                char *tmp_str = duration_str_get(info->recstart, info->recstart + info->duration);
                if((ptime != NULL) && (size > 0))
                {
                    int tmp_len_str = strlen(tmp_str);
                    if(tmp_len_str > (size - 1))
                        tmp_len_str = size - 1;
                    memcpy(ptime, tmp_str, tmp_len_str);
                    *title = GxCore_Calloc(1, strlen(tmp_title) + 8);
                    if(*title)
                    {
                        sprintf(*title, "%s", tmp_title);
                    }
                }
                else
                {
                    *title = GxCore_Calloc(1, strlen(tmp_str) + strlen(tmp_title) + 8);
                    if(*title)
                    {
                        if(ctrl->wgt.reverse_flag)
                            sprintf(*title, "%s %s", tmp_title, tmp_str);
                        else
                            sprintf(*title, "%s %s", tmp_str, tmp_title);
                    }
                }
            }
            else
            {
                *title = GxCore_Strdup(tmp_title);
            }
        }
        else
        {
            *title = NULL;
        }
    }

    // channnel_logo
    if(chlogo)
    {
        char *dir_path = NULL, *logo_path = NULL;
        if((dir_path = _get_dvr_dir_path_by_index(cur_sel)) != NULL)
            logo_path = app_cover_pvr_get_full_name2(dir_path, PVR_CHLOGO_NAME);
        APP_FREE(dir_path);
        if(logo_path && GxCore_FileExists(logo_path) == GXCORE_FILE_EXIST)
        {
            *chlogo = logo_path;
        }
        else
        {
            APP_FREE(logo_path);
        }
    }

    return 0;
}

void app_cover_pvr_event_name_and_logo_free(char **title, char **chlogo)
{
    if(title)
        APP_FREE(*title);
    if(chlogo)
        APP_FREE(*chlogo);
}

static void _cover_pvr_play(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    explorer_para explorer;
    GxDirent *ents = NULL;
    int i = 0;

    if((ctrl->nuses == 0) || (ctrl->uses == NULL))
        return;

    if((ents = GxCore_Calloc(ctrl->nuses, sizeof(GxDirent))) == NULL)
    {
        COVER_PVR_ERR("malloc failed!\n");
        return;
    }
    for(i = 0; i < ctrl->nuses; i++)
        memcpy(&ents[i], &ctrl->paras.ents[ctrl->uses[i]], sizeof(GxDirent));

#if PVR_PREVIEW_SUPPORT
    APP_TIMER_REMOVE(ctrl->preview_timer);
    _cover_pvr_preview_stop(NULL);
#endif
    memset(&explorer, 0, sizeof(explorer_para));
    explorer.path = ctrl->path;
    explorer.suffix = PVR_INDEX_SUFFIX;
    explorer.nents = ctrl->nuses;
    explorer.ents = ents;
    play_list_init(PLAY_LIST_TYPE_MOVIE, &explorer, ctrl->cur_index);
    APP_FREE(ents);

    GUI_CreateDialog(WIN_MOVIE_VIEW);
}

static void _cover_pvr_quit(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    if(ctrl->screen_flag == 1)
    {
        GUI_EndDialog("after wnd_full_screen");
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        GUI_SetProperty(WND_FULL_SCREEN, "draw_now", NULL);

        if(g_AppPlayOps.normal_play.play_total != 0)
        {
            app_play_current_prog(PLAY_TYPE_NORMAL | PLAY_MODE_POINT);
        }
        ctrl->screen_flag = 0;
    }
    else
    {
        if(strcmp(GUI_GetFocusWindow(), ctrl->wgt.wnd_name) != 0)
        {
            GUI_EndDialog("after wnd_cover_pvr");
        }
        GUI_EndDialog(ctrl->wgt.wnd_name);
    }
}

status_t app_cover_pvr_hotplug(char *hotplug_dev)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char dev_name[PVR_DEV_STR_LEN + 1] = { 0 };
    int dev_len;
    char *p = NULL;

    strncpy(dev_name, ctrl->partition.dev_name, PVR_DEV_STR_LEN);
    dev_len = strlen(dev_name);
    p = dev_name + dev_len - 1;
    if(*p >= '0' && *p <= '9')  // "/dev/usb0/1" remove partition num
        *p = '\0';

    if((hotplug_dev != NULL) && strncmp(dev_name, hotplug_dev, strlen(hotplug_dev)) == 0)
    {
        if(GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS)
        {
            GxMsgProperty_PlayerStop player_stop;
            player_stop.player = PMP_PLAYER_AV;
            app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);
        }
        _cover_pvr_quit();
        pvr_partition_get(NULL);
    }

    return GXCORE_SUCCESS;
}

static int s_del_sel = 0;
static int _delete_cover_pvr_update(PopDlgRet ret)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    _cover_pvr_ctrl_filter();
    if(GUI_CheckDialog(ctrl->wgt.wnd_name) == GXCORE_SUCCESS)
    {
        if(s_del_sel >= ctrl->nuses)
        {
            s_del_sel = ctrl->nuses - 1;
        }
        _cover_pvr_update_item_all(-1, s_del_sel, true);
    }
    return 0;
}

static status_t _delete_cover_pvr(int index)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    status_t ret = GXCORE_ERROR;
    DirPara dir_para;
    char *dir_path = NULL;
    char *sub_path = NULL;
    char *dvr_path = NULL;
    int i = 0;

    memset(&dir_para, 0, sizeof(dir_para));
    if((ctrl->nuses == 0) || (ctrl->uses == NULL))
        return GXCORE_ERROR;

    if((dir_path = _get_dvr_dir_path_by_index(index)) == NULL)
        return GXCORE_ERROR;
#ifdef LINUX_OS
    struct stat st = { 0 };
    if(lstat(dir_path, &st) < 0 || (S_ISDIR(st.st_mode) && access(dir_path, W_OK) < 0) || !S_ISDIR(st.st_mode))
    {
        APP_FREE(dir_path);
        return GXCORE_ERROR;
    }
#endif

    dir_para.nents = GxCore_GetDir(dir_path, &(dir_para.ents), NULL);
    if(dir_para.nents > 0)
    {
        for(i = 0; i < dir_para.nents; i++)
        {
            sub_path = app_cover_pvr_get_full_name2(dir_path, dir_para.ents[i].fname);
            if(sub_path)
            {
                if((ret = GxCore_FileDelete(sub_path)) == GXCORE_ERROR)
                    goto end;
                APP_FREE(sub_path);
            }
        }
    }
    ret = (GxCore_FileExists(dir_path) == GXCORE_FILE_UNEXIST) ? (GXCORE_SUCCESS) : (GxCore_Rmdir(dir_path));
    if(ret == GXCORE_ERROR)
        goto end;

    //////delete xxx.ts.dvr///////
    dvr_path = app_cover_pvr_get_full_name2(ctrl->path, ctrl->paras.ents[ctrl->uses[index]].fname);
    if(dvr_path)
    {
        if((ret = GxCore_FileDelete(dvr_path)) == GXCORE_ERROR)
            goto end;
    }

#ifdef MODIFY_FATMOUNT
    extern void modify_disk_mount(void);
    modify_disk_mount();
#endif

    ctrl->paras.attrs[ctrl->uses[index]].flag |= FILE_REMOVED;
end:
    app_free_dir_ent(&dir_para.ents, &dir_para.nents);
    APP_FREE(dir_path);
    APP_FREE(sub_path);
    APP_FREE(dvr_path);
    return ret;
}

static int _delete_cover_pvr_progress(int first, int total)
{
    status_t result = GXCORE_SUCCESS;
    int sel = first;
    int cnt = total;
    PopDlg pop = { 0 };

    s_del_sel = first;
    pop.type = POP_TYPE_NO_BTN;
    pop.mode = POP_MODE_UNBLOCK;
    pop.format = POP_FORMAT_POPUP;
    pop.str = STR_ID_DELETING;
    pop.timeout_sec = 20 * total;
    popdlg_create(&pop);
    GUI_SetInterface("flush", NULL);

    while(cnt)
    {
        if((result = _delete_cover_pvr(sel)) != GXCORE_SUCCESS)
            break;
        cnt--, sel++;
    }

    popdlg_destroy();
    if(result == GXCORE_SUCCESS)
    {
        _delete_cover_pvr_update(POP_VAL_OK);
    }
    else
    {
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_DEL_FAIL;
        pop.timeout_sec = 2;
        pop.exit_cb = (total > 1) ? _delete_cover_pvr_update : NULL;
        popdlg_create(&pop);
    }

    return 0;
}

static int _delete_one_cover_pvr_poptip_cb(PopDlgRet ret)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    if(ret == POP_VAL_OK)
    {
        _delete_cover_pvr_progress(ctrl->cur_index, 1);
    }
    return 0;
}

static int _delete_all_cover_pvr_poptip_cb(PopDlgRet ret)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    if(ret == POP_VAL_OK)
    {
        _delete_cover_pvr_progress(0, ctrl->nuses);
    }
    return 0;
}

static void _delete_cover_pvr_pop_create(bool del_one)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    if(ctrl->nuses > 0)
    {
        PopDlg pop = { 0 };
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        if(del_one)
        {
            pop.str = STR_ID_SURE_DELETE;
            pop.exit_cb = _delete_one_cover_pvr_poptip_cb;
        }
        else
        {
            pop.str = STR_ID_SURE_DELETE_ALL;
            pop.exit_cb = _delete_all_cover_pvr_poptip_cb;
        }
        popdlg_create(&pop);
    }
}

static int _cover_pvr_del_all_passwd_dlg_cb(void *usr_data)
{
    _delete_cover_pvr_pop_create(false);
    return 0;
}

int app_cover_pvr_manage_init(bool on_fullscreen, bool reverse_flag, const char *menu_logo)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    if(ctrl->mgr.mutex == 0)
    {
        ctrl->focus_sel = -1;  // init value must be -1.
        ctrl->rfocus_sel = -1;
        GxCore_MutexCreate(&ctrl->mgr.mutex);
    }
    while(_cover_pvr_thread_run_check())
        GxCore_ThreadDelay(10);
    GxCore_MutexLock(ctrl->mgr.mutex);
    ctrl->mgr.first_in_flag = 3;
    ctrl->mgr.rebuild_flag = 0;
    ctrl->mgr.logo_copy_rq = 0;
    ctrl->mgr.logo_stop_rq = 0;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    ctrl->screen_flag = (uint8_t)on_fullscreen;
    ctrl->detail_flag = 0;
    ctrl->filter_sel = 0;
    ctrl->cur_index = 0;
    ctrl->menu_logo = menu_logo?GxCore_Strdup(menu_logo):NULL;

    ctrl->wgt.reverse_flag = reverse_flag;
    ctrl->wgt.wnd_name = WND_COVER_PVR;
    ctrl->wgt.img_menu_logo = "img_cover_pvr_company_logo";
    ctrl->wgt.group_item_common = "btn_cover_pvr_group%d";
    ctrl->wgt.item_back_common = "img_cover_pvr_back%d";
    ctrl->wgt.item_logo_common = "img_cover_pvr_logo%d";
#if PVR_PREVIEW_SUPPORT
    ctrl->wgt.item_preview_common = "text_cover_pvr_pp_ch%d";
#endif
    ctrl->wgt.item_title_common = "text_cover_pvr_title%d";
    ctrl->wgt.item_time_common = "text_cover_pvr_time%d";
    ctrl->wgt.item_ch_common = "text_cover_pvr_ch%d";

    ctrl->wgt.text_item_filter = "text_cover_pvr_content_filter";
    ctrl->wgt.text_item_page = "text_cover_pvr_page";

    ctrl->wgt.text_item_duration = "text_cover_pvr_duration";
    ctrl->wgt.text_item_name = "text_cover_pvr_name";
    ctrl->wgt.text_item_parent = "text_cover_pvr_parent";
    ctrl->wgt.text_item_genres = "text_cover_pvr_genres";
    ctrl->wgt.text_item_brief = "text_cover_pvr_brief";

    ctrl->wgt.text_ch_name = "text_cover_pvr_ch_name";
    ctrl->wgt.text_ch_logo = "text_cover_pvr_ch_logo";
    ctrl->wgt.img_ch_logo = "img_cover_pvr_ch_logo";

    ctrl->wgt.text_item_size = "text_cover_pvr_size";
    ctrl->wgt.text_item_dir = "text_cover_pvr_dir";
    ctrl->wgt.text_item_file = "text_cover_pvr_file";

    ctrl->wgt.details_back = "img_cover_pvr_details_back";
    ctrl->wgt.details_duration = "text_cover_pvr_details_duration";
    ctrl->wgt.details_title = "text_cover_pvr_details_title";
    ctrl->wgt.details_notepad = "notepad_cover_pvr_details";
    ctrl->wgt.details_exit = "btn_cover_pvr_details_exit";

    ctrl->wgt.img_ok_tip = "img_cover_pvr_ok_tip";
    ctrl->wgt.txt_ok_tip = "text_cover_pvr_ok_tip";
    ctrl->wgt.text_time = "text_cover_pvr_sys_time";
    ctrl->wgt.text_date = "text_cover_pvr_sys_date";

    return 0;
}

int app_cover_pvr_create_dialog(void)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    GUI_CreateDialog(ctrl->wgt.wnd_name);
    return 0;
}

void app_cover_pvr_init(void)
{
	GUI_LinkDialog(WND_COVER_PVR, "cover_pvr/wnd_cover_pvr.xml");
	GUI_LinkDialog(WND_COVER_PVR_INFO, "cover_pvr/wnd_cover_pvr_info.xml");
}

static int _cover_pvr_num_reponse(unsigned int num_value)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    int orig_sel = 0, sel = 0, total = 0;

    if(ctrl->nuses == 0)
        return -1;

    orig_sel = ctrl->cur_index / PVR_ITEMS_PER_PAGE;
    sel = num_value - 1;
    total = (ctrl->nuses - 1) / PVR_ITEMS_PER_PAGE + 1;

    if((sel < total && sel >= 0) && sel != orig_sel)
    {
        if(ctrl->detail_flag == 1)
            _cover_pvr_details_hide();
        _cover_pvr_update_item_all(ctrl->cur_index, sel * PVR_ITEMS_PER_PAGE, true);
    }

    return 0;
}

static int _cover_pvr_create(GuiWidget *widget, void *usrdata)
{
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    char wgt[ITEM_COMMON_SIZE] = { 0 };
    int32_t init_value = 0;

    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    if (ctrl->menu_logo)
    {
        GUI_SetProperty(ctrl->wgt.img_menu_logo, "img", ctrl->menu_logo);
    }
    else
    {
        GUI_SetProperty(ctrl->wgt.img_menu_logo, "img", STR_ID_BLANK);
    }

#if FOCUS_GROUP_DEFAULT
    _cover_pvr_update_group_all(-1, 0, AREA_GROUP);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, 1);
    GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
#else
    _cover_pvr_update_group_all(-1, 0, AREA_ITEM);
    int focus_sel = (ctrl->wgt.reverse_flag) ? ctrl->rfocus_sel : ctrl->focus_sel;
    if(focus_sel == 0 && ctrl->nuses > 0)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, 1);
        GUI_SetProperty(wgt, "img", ITEM_FOCUS_IMG);
    }
#endif
    _cover_pvr_change_ok_tip();
    ctrl->func = app_number_respones_ex_func_get();
    app_number_respones_ex_func_register(_cover_pvr_num_reponse);
#if PVR_PREVIEW_SUPPORT
    if(ctrl->area_sel == AREA_ITEM && ctrl->nuses > 0)
    {
        APP_TIMER_ADD(ctrl->preview_timer, _cover_pvr_preview, 1000, TIMER_REPEAT);
        APP_TIMER_ADD(ctrl->preview_stop_timer, _cover_pvr_preview_stop, 30000, TIMER_REPEAT);
    }
#endif

    return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_destroy(GuiWidget *widget, void *usrdata)
{
    int32_t init_value = 0;
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    app_number_respones_ex_func_register(ctrl->func);
    _cover_pvr_ctrl_release();
    play_movie_stop();
    pmpset_exit();
    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

    _cover_pvr_thread_stop_set();
#if TIMER_FLUSH_LOGO
    _cover_pvr_clean_logo_data(false);
#endif

    return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode, event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                else
                    _cover_pvr_quit();
                break;
            case STBK_OK:
                if(ctrl->area_sel == AREA_ITEM)
                {
                    if(ctrl->detail_flag == 1)
                        _cover_pvr_details_hide();
                    _cover_pvr_play();
                }
#if MORE_GROUP_SUPPORT
                else if(ctrl->area_sel == AREA_MORE)
                {
                    if(ctrl->detail_flag == 1)
                        _cover_pvr_details_hide();
                    _cover_pvr_ctrl_filter_pop(AREA_GROUP);
                }
#endif
                break;

            case STBK_RED:
                if(ctrl->nuses == 0)
                    break;
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                _delete_cover_pvr_pop_create(true);  // delete one
                break;
            case STBK_GREEN:
                if(ctrl->nuses == 0)
                    break;
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                app_passwd_dlg_operate(_cover_pvr_del_all_passwd_dlg_cb);  // delete all
                break;

            case STBK_YELLOW:
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                if(ctrl->area_sel == AREA_ITEM)
                    _cover_pvr_ctrl_filter_pop(AREA_ITEM);
                else
                    _cover_pvr_ctrl_filter_pop(AREA_GROUP);
                break;
            case STBK_BLUE:
                if(ctrl->cur_index < PVR_ITEMS_PER_PAGE)
                    break;
                if(ctrl->detail_flag == 1)
                    _cover_pvr_details_hide();
                if(ctrl->nuses)
                    _cover_pvr_update_item_all(-1, 0, true);
                break;
            case STBK_INFO:
                if(ctrl->detail_flag == 0)
                    _cover_pvr_details_show();
                else
                    _cover_pvr_show_no_info_popdlg();
                break;
            case STBK_UP:
            case STBK_DOWN:
            case STBK_LEFT:
            case STBK_RIGHT:
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
                if(ctrl->detail_flag == 0)
                {
                    _cover_pvr_change_keypress(event->key.sym);
                }
                else
                {
                    _cover_pvr_details_keypress(event->key.sym, ctrl->wgt.details_notepad);
                }
                break;
            case STBK_2:
            case STBK_3:
            case STBK_4:
            case STBK_5:
            case STBK_6:
            case STBK_7:
            case STBK_8:
            case STBK_9:
            case STBK_0:
                GUI_EndDialog(WND_VOLUME);
                GUI_CreateDialog(WND_NUMBER);
                GUI_SendEvent(WND_NUMBER, event);
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cover_pvr_create(GuiWidget *widget, void *usrdata)
{
    return _cover_pvr_create(widget, usrdata);
}

SIGNAL_HANDLER int app_cover_pvr_destroy(GuiWidget *widget, void *usrdata)
{
    return _cover_pvr_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_cover_pvr_got_focus(GuiWidget *widget, void *usrdata)
{
#if PVR_PREVIEW_SUPPORT
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;

    if (ctrl->nuses > 0 && ctrl->area_sel == AREA_ITEM) {
        APP_TIMER_ADD(ctrl->preview_timer, _cover_pvr_preview, 1000, TIMER_REPEAT);
        APP_TIMER_ADD(ctrl->preview_stop_timer, _cover_pvr_preview_stop, 30000, TIMER_REPEAT);
    }
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cover_pvr_lost_focus(GuiWidget *widget, void *usrdata)
{
#if PVR_PREVIEW_SUPPORT
    CoverPvrCtrl *ctrl = &s_cover_pvr_ctrl;
    
    APP_TIMER_REMOVE(ctrl->preview_timer);
    _cover_pvr_preview_stop(NULL);
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cover_pvr_keypress(GuiWidget *widget, void *usrdata)
{
    return _cover_pvr_keypress(widget, usrdata);
}

#if PVR_PREVIEW_SUPPORT
#if 0
static void _cover_pvr_show_preview_failed_popdlg(void)
{
    PopDlg pop = { 0 };
    pop.type = POP_TYPE_OK;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = STR_ID_PREVIEW_FAILED;
    pop.timeout_sec = 3;
    popdlg_create(&pop);
}
#endif

static void _cover_pvr_play_status(GxMessage* msg)
{
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    if(GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS || (player_status == NULL) || (strcmp((char*)(player_status->player), PMP_PLAYER_AV) != 0))
    {
        return;
    }

	switch(player_status->status)
    {
		case PLAYER_STATUS_ERROR:
		case PLAYER_STATUS_PLAY_END:
            if (player_status->status == PLAYER_STATUS_ERROR)
            {
                app_log_info("PVR recording file preview failed, status: %d\n", player_status->status);
            }
            _cover_pvr_preview_stop(NULL);
            break;
        default:
            break;
    }
}

int app_cover_pvr_play_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			_cover_pvr_play_status(msg);
			break;
    }
	return EVENT_TRANSFER_STOP;
}
#endif
#endif
