#include "app.h"
#include "app_module.h"
#include "app_windows.h"
#ifdef SUBTTRANS_SUPPORT
#include "module/subtitle/gx_subtitle_module.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif
#include "gx_subttrans.h"
#include "gx_subttrans_pool.h"
#include "gx_subttrans_demux.h"

#define STAPI_INIT_DEFAULT (0x11112222)
#define SUBT_SHOW_TIME (3)//s
#define SUBT_SERVER_MAX (4)
#define ST_DRAWING_SUBT_STR_MAX (512)

typedef struct _stapi_params
{
    int init;
    uint32_t elem_pid;
    uint16_t service_id;
    uint8_t lang[SUBT_LANG_MAX];
    uint8_t type;
}stapi_params_t;

typedef struct _subt_prepare_info
{
    GxST_TransParams_t param;
    handle_t   thread_id;
    int        is_download;
}stPreInfo_t;


static stapi_params_t st_params = {0};
static STDrawInfo_t st_drawing_data = {0};
static STDrawInfo_t st_wait_show = {0};
static event_list* st_trans_timer = NULL;
static event_list* st_subt_showend_timer = NULL;
static stPreInfo_t st_PreInfoList[SUBT_SERVER_MAX];
static char wait_show_flag = 0;

static char *gx_subttrans_langlist[] = {"Arabic", "Englist", "French"};
#define SUBTTRANS_LANG_NUM (sizeof(gx_subttrans_langlist) / sizeof(char *))

static char *gx_subttrans_textsize[] = {"Normal", "Smail", "Large"};
#define SUBTTRANS_TEXTSIZE_NUM (sizeof(gx_subttrans_textsize) / sizeof(char *))

static char *gx_subttrans_fontcolor[] = {"Black", "Yellow", "White", "Blue", "Green", "Red"};
#define SUBTTRANS_FONTCOLOR_NUM (sizeof(gx_subttrans_fontcolor) / sizeof(char *))

static char *gx_subttrans_backgroudcolor[] = {"White", "Black", "Yellow", "Green", "Red", "Blue"};
#define SUBTTRANS_BACKGROUDCOLOR_NUM (sizeof(gx_subttrans_backgroudcolor) / sizeof(char *))

static void subttrans_translation_info_prepare_init(void);
extern bool app_check_av_running(void);

static bool app_st_check_need_show(void)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
        || (GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG)))
        return false;

    bool avruning =  app_check_av_running();
    return avruning;
}
static void gx_st_subt_draw_set_config(void)
{
    int font_size=0, font_col=0, bg_col=0;
    GxBus_ConfigGetInt(ST_FONTSIZE_SEL_KEY, &font_size, ST_FONTSIZE_SEL);
    GxBus_ConfigGetInt(ST_FONTCOLOR_SEL_KEY, &font_col, ST_FONTCOLOR_SEL);
    GxBus_ConfigGetInt(ST_BAGKCOLOR_SEL_KEY, &bg_col, ST_BAGKCOLOR_SEL);
    Gx_SubtTrans_SubtShowSet(font_size, font_col, bg_col);
}

static int s_subt_show_flag = 0;
static int gx_subt_is_show(void)
{
    return s_subt_show_flag;
}
static void gx_st_subt_draw_show(char *subt_str)
{
#if OSD_SUBTITLE_SUPPORT
    extern int GxSubtitleShowTextDraw(handle_t handle, GxSubtitleShowTextPage *txt);
    GxSubtitleShowTextPage txt;
    if(!subt_str)
        return;
    memset(&txt, 0, sizeof(GxSubtitleShowTextPage));
    txt.attr = 1;
    txt.lines.lines = 1;
    txt.lines.text[0] = subt_str;
    GxSubtitleShowTextDraw(0, &txt);
#endif
    s_subt_show_flag = 1;
}
static void gx_st_subt_draw_clean(void)
{
#if OSD_SUBTITLE_SUPPORT
    extern int GxSubtitleShowTextClean(handle_t handle);
    GxSubtitleShowTextClean(0);
#endif
    s_subt_show_flag = 0;
}
static void gx_st_subt_draw_resume(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_resume();
#endif
}
static void gx_st_subt_draw_pause(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_pause();
#endif
}
static void gx_st_subt_draw_init(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_init();
#endif
}
static void gx_st_subt_draw_deinit(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_deinit();
#endif
}

void Gx_SubtTrans_DrawClean(void)
{
    gx_st_subt_draw_clean();
    wait_show_flag = 0;
}
void Gx_SubtTrans_DrawPause(void)
{
    gx_st_subt_draw_pause();
}
void Gx_SubtTrans_DrawResume(void)
{
    gx_st_subt_draw_resume();
}

//转ISO 639-1
static char *gx_st_api_lang_isolang(char *lang3)
{
    char *lang2 = NULL;
    if(!lang3)
        return NULL;
    if((0 == strcmp(lang3, "pol")) || (0 == strcmp(lang3, "Polish")))
        lang2 = ST_LANG639_POL;
    else if((0 == strcmp(lang3, "eng")) || (0 == strcmp(lang3, "Englist")))
        lang2 = ST_LANG639_ENG;
    else if((0 == strcmp(lang3, "ara")) || (0 == strcmp(lang3, "Arabic")))
        lang2 = ST_LANG639_ARA;
    else if((0 == strcmp(lang3, "fre")) || (0 == strcmp(lang3, "fra")) ||(0 == strcmp(lang3, "French")))
        lang2 = ST_LANG639_FRE;
    else if((0 == strcmp(lang3, "chi")) || (0 == strcmp(lang3, "Chinese")))
        lang2 = ST_LANG639_ZH;
    else if((0 == strcmp(lang3, "deu")) || (0 == strcmp(lang3, "German")))
        lang2 = ST_LANG639_DEU;
    else
        lang2 = ST_LANG639_ENG;

    return lang2;
}

static char *gx_st_api_get_trans_lang(void)
{
    int lang_sel = 0;
    GxBus_ConfigGetInt(ST_LANG_SEL_KEY, &lang_sel, ST_LANG_SEL);
    return gx_st_api_lang_isolang(gx_subttrans_langlist[lang_sel]);
}

static int _stapi_check_pid_valid(uint32_t pid)
{
    if((pid >= 0x1FFF) || (pid <= 0))
        return -1;
    return 0;
}

static void gx_subttrans_subt_showend_stop(void)
{
    if(st_subt_showend_timer)
    {
        remove_timer(st_subt_showend_timer);
        st_subt_showend_timer = NULL;
    }
}

static int st_trans_subt_showend_timeout(void *usrdata)
{
    if(1 == gx_subt_is_show())
    {
        gx_st_subt_draw_clean();
    }
    gx_subttrans_subt_showend_stop();
    return 0;
}

static void gx_subttrans_subt_showend_start(void)
{
    gx_subttrans_subt_showend_stop();
    st_subt_showend_timer = create_timer(st_trans_subt_showend_timeout, SUBT_SHOW_TIME*1000, NULL, TIMER_REPEAT);
}

static void st_translated_subt_showing(char *subt_data, int pts)
{
    if((st_params.init != STAPI_INIT_DEFAULT) || (!subt_data))
        return;
    if(1 == gx_subt_is_show()) {
        gx_st_subt_draw_clean();
    }
    st_drawing_data.pts = pts;
    memset(st_drawing_data.subt_data, 0 , ST_DRAWING_SUBT_STR_MAX);
    strncpy(st_drawing_data.subt_data, subt_data, ST_DRAWING_SUBT_STR_MAX-1);
    gx_st_subt_draw_show(st_drawing_data.subt_data);
    gx_subttrans_subt_showend_start();
    ST_API_PRINTF("===> DRAW[pts=%f] %s\n", st_drawing_data.pts,st_drawing_data.subt_data);
}

static int _st_trans_subt_show_timeout(void)
{
    static int wait_show_count = 0; // 处理长期不到时间的subt，可能是个错误pts.
    uint64_t pts_subt = 0;
    int pts_a = 0, pts_s = 0;
    char *subt_data = NULL;
    PlayerSyncInfo info = {0};

    ST_API_PRINTF("wait_show_flag=%d, g_STDrawPool.get_num=%d\n",  wait_show_flag, g_STDrawPool.get_num());
    if((wait_show_flag == 0) && (g_STDrawPool.get_num() <= 0))
        return 0;

    GxPlayer_MediaGetSyncInfo(PLAYER_FOR_NORMAL, &info);
    pts_a = info.apts;

    if(wait_show_flag == 0)
    {
        g_STDrawPool.get(&pts_subt, &subt_data);
        pts_s = (int32_t)(pts_subt>>1);
        pts_s /= 45;
        if(pts_s >= pts_a-200 && pts_s <= pts_a+200) {
            // show
            st_translated_subt_showing(subt_data, pts_s);
            APP_FREE(subt_data);
            wait_show_flag = 0;
        }else if(pts_s > pts_a + 200) { // wait to show
            wait_show_flag = 1;
            wait_show_count = 0;
            st_wait_show.subt_data = subt_data;
            st_wait_show.pts = pts_s;
        }else { //subt expired
            app_log_info("!!! Expired Subt [delay:%d]: %s\n", pts_a-pts_s, subt_data);
            st_translated_subt_showing(subt_data, pts_s);
            APP_FREE(subt_data);
            wait_show_flag = 0;
        }
    }
    else
    {
        wait_show_count ++;
        if(wait_show_count > 50) {// 10s
            wait_show_flag = 0;
            wait_show_count = 0;
            APP_FREE(st_wait_show.subt_data);
            st_wait_show.pts = 0;
            app_log_error("!!!! Err pts Subt\n");
            return 0;
        }
        if(st_wait_show.pts >= pts_a - 200 && st_wait_show.pts <= pts_a + 200) {
            //show wait_showsubt_data
            st_translated_subt_showing(st_wait_show.subt_data, st_wait_show.pts);
            APP_FREE(st_wait_show.subt_data);
            st_wait_show.pts = 0;
            wait_show_flag = 0;
        }else if(st_wait_show.pts < (pts_a-200)) {
            app_log_info("!!!! Expired Subt [delay:%d]: %d\n", pts_a-st_wait_show.pts, st_wait_show.pts-pts_a);
            st_translated_subt_showing(st_wait_show.subt_data, st_wait_show.pts);
            APP_FREE(st_wait_show.subt_data);
            st_wait_show.pts = 0;
            wait_show_flag = 0;
        }
    }
    return 0;
}

static void gx_subttrans_data_timer_stop(void)
{
    if(st_trans_timer)
    {
        remove_timer(st_trans_timer);
        st_trans_timer = NULL;
    }
    wait_show_flag = 0;
}

static int st_trans_data_timer_timeout(void *usrdata)
{
    if(true == app_st_check_need_show())
        _st_trans_subt_show_timeout(); // for show subt
    return 0;
}

static void gx_subttrans_data_timer_start(void)
{
    gx_subttrans_data_timer_stop();
    st_trans_timer = create_timer(st_trans_data_timer_timeout, 200, NULL, TIMER_REPEAT);
}

void Gx_SubtTransInit(void)
{
    memset(&st_params, 0, sizeof(st_params));
    st_params.init = STAPI_INIT_DEFAULT;
    Gx_SubtTrans_SiInit(1, 0); // Demux1&TsSource0
    subttrans_translation_info_prepare_init();
}

void Gx_SubtTransStart(void)
{
    int enable_trans = 1;
    GxBus_ConfigSetInt(ST_TRANS_ENABLE_KEY, enable_trans);

    if(st_params.init != STAPI_INIT_DEFAULT)
        return;

    Gx_SubtTrans_SiPesStart(st_params.elem_pid, st_params.service_id, st_params.type, (char *)st_params.lang);
    gx_st_subt_draw_init();
    gx_st_subt_draw_set_config();
    st_drawing_data.pts = 0;
    APP_FREE(st_drawing_data.subt_data);
    st_drawing_data.subt_data = GxCore_Calloc(1, ST_DRAWING_SUBT_STR_MAX);
    g_STDrawPool.init();
    gx_subttrans_data_timer_start();

}

void Gx_SubtTransStop(void)
{
    int enable_trans = 0;
    GxBus_ConfigSetInt(ST_TRANS_ENABLE_KEY, enable_trans);

    if(st_params.init != STAPI_INIT_DEFAULT)
        return;

    Gx_SubtTrans_SiPesStop();
    gx_subttrans_data_timer_stop();
    gx_subttrans_subt_showend_stop();
    gx_st_subt_draw_deinit();
    APP_FREE(st_drawing_data.subt_data);
    st_drawing_data.pts = 0;
    g_STDrawPool.destory();
}

void Gx_SubtTransDestory(void)
{
    Gx_SubtTransStop();
    Gx_SubtTrans_SiDestory();
    memset(&st_params, 0, sizeof(st_params));
    subttrans_translation_info_prepare_init();
}

void Gx_SubtTrans_SetSubtPid(int pid, char *lang, uint16_t service_id, uint8_t subt_type)
{
    if((-1 == _stapi_check_pid_valid(pid)) || !lang || (st_params.init != STAPI_INIT_DEFAULT))
        return;

    st_params.elem_pid = pid;
    st_params.service_id = service_id;
    st_params.type = subt_type;
    memcpy(st_params.lang, lang, SUBT_LANG_MAX);
}

int Gx_SubtTransGetStatus(void)
{
    int enable_trans = 0;
    if(GXCORE_SUCCESS != Gx_SubtTransActivate_Get_Status(NULL, 0)) {
        GxBus_ConfigSetInt(ST_TRANS_ENABLE_KEY, enable_trans);
        return 0;
    }
    GxBus_ConfigGetInt(ST_TRANS_ENABLE_KEY, &enable_trans, ST_TRANS_ENABLE_DEFAULT);
    return enable_trans;
}

int Gx_SubtTrans_ChannelSwitch(void)
{
    if((st_params.init != STAPI_INIT_DEFAULT) || 0 == Gx_SubtTransGetStatus())
        return -1;

    Gx_SubtTrans_SiPesStop();
    Gx_SubtTrans_SiPesStart(st_params.elem_pid, st_params.service_id, st_params.type, (char *)st_params.lang);
    return -1;
}

char **Gx_SubtTrans_SubtLangGet(int *num)
{
    *num = SUBTTRANS_LANG_NUM;
    return gx_subttrans_langlist;
}
char **Gx_SubtTrans_SubtFontGet(int *num)
{
    *num = SUBTTRANS_TEXTSIZE_NUM;
    return gx_subttrans_textsize;
}
char **Gx_SubtTrans_SubtFontColorGet(int *num)
{
    *num = SUBTTRANS_FONTCOLOR_NUM;
    return gx_subttrans_fontcolor;
}
char **Gx_SubtTrans_SubtBackgroudColorGet(int *num)
{
    *num = SUBTTRANS_BACKGROUDCOLOR_NUM;
    return gx_subttrans_backgroudcolor;
}

void Gx_SubtTrans_SubtShowSet(int font_size, int fontcolor, int bgcolor)
{
#if OSD_SUBTITLE_SUPPORT
    OsdSubtFont font;
    OsdSubtColor color;

    if(font_size == 0)
        font = OSDSUBT_FONT_PROG;
    else if(font_size == 1)
        font = OSDSUBT_FONT_22;
    else
        font = OSDSUBT_FONT_MAIN;
    app_osd_subt_font_set(font);

    memset(&color, 0, sizeof(OsdSubtColor));
    if(fontcolor == 0)
        color.forecolor = 0x000000;  // Black
    else if(fontcolor == 1)
        color.forecolor = 0xf8d616;  // Yellow
    else if(fontcolor == 2)
        color.forecolor = 0xf4f4f4;  // White
    else if(fontcolor == 3)
        color.forecolor = 0x270fcb;  // Blue
    else if(fontcolor == 4)
        color.forecolor = 0x0b9520;  // Green
    else
        color.forecolor = 0xFF0000;  // Red

    if(bgcolor == 0)
        color.backcolor = 0xdddddd;  // White
    else if(bgcolor == 1)
        color.backcolor = 0x000000;  // Black
    else if(bgcolor == 2)
        color.backcolor = 0xcfcf51;  // Yellow
    else if(bgcolor == 3)
        color.backcolor = 0xa0ee99;  // Green
    else if(bgcolor == 4)
        color.backcolor = 0xb01936;  // Red
    else
        color.backcolor = 0x6e84e3;  // Blue
    app_osd_subt_color_set(color);
#endif
}

int Gx_SubtTrans_GetDelayms(void)
{
    int time = 0;
    if(0 == Gx_SubtTransGetStatus())
        return 0;

    GxBus_ConfigGetInt(ST_DELAY_KEY, &time, ST_DELAY_DEFAULT);
    return time*1000;
}

void Gx_SubtTrans_Set_CustomInfo(char *oem, char *brand, char *mac)
{
    if(!oem || !brand || !mac)
        return;
    GxBus_ConfigSet(ST_LOGIN_OEM_KEY, oem);
    GxBus_ConfigSet(ST_LOGIN_BRAND_KEY, brand);
    GxBus_ConfigSet(ST_LOGIN_MAC_KEY, mac);
}

status_t Gx_SubtTrans_Replay_ByTsbuff(void)
{
#define DELAY_KEY "tsbuff:"
    status_t ret = GXCORE_ERROR;
    char url_temp[PLAYER_URL_LONG+1] = {0};
    int time  = 0;
    PlayerProgInfo  *info = app_player_prog_info_get();
    if(GXCORE_SUCCESS != GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info))
        return  GXCORE_ERROR;

    char* pos = strstr(info->url, DELAY_KEY);
    if(0 >= (time = Gx_SubtTrans_GetDelayms()))
    {
        if(pos) {
            char *end = strchr(pos, '&');
            strncat(url_temp, info->url, pos-info->url);
            if(end) {
                strcat(url_temp+(pos-info->url), end+1);
            }
            char *pos2 = strstr(url_temp, "tsbuff_dmxid");
            if(pos2) {
                char url_temp2[PLAYER_URL_LONG+1] = {0};
                strncat(url_temp2, url_temp, pos2-&url_temp[0]);
                char *end = strchr(pos2, '&');
                if(end) {
                    strcat(url_temp2+(pos2-&url_temp[0]), end+1);
                }
                memset(&url_temp, 9, PLAYER_URL_LONG+1);
                strcpy(url_temp, url_temp2);
            }
        }
        else
        {
            return ret;
        }
        ret = GXCORE_ERROR;
    }
    else
    {
        if(pos != NULL)
        {
            char *start = pos + strlen(DELAY_KEY);
            char *end = strchr(start, '&');
            if(start != NULL) {
                size_t prefix_length = pos - info->url + strlen(DELAY_KEY);
                strncat(url_temp, info->url, prefix_length);
                if(end != NULL) {
                    snprintf(url_temp+prefix_length, PLAYER_URL_LONG-prefix_length, "%d%s", time, end);
                }else{
                    snprintf(url_temp+prefix_length, PLAYER_URL_LONG-prefix_length, "%d", time);
                }
            }
        }
        else
        {
            snprintf(url_temp, PLAYER_URL_LONG, "%s&%s%d&tsbuff_dmxid:%d", info->url, DELAY_KEY, time, TS_2_DMX);
        }
        ret = GXCORE_SUCCESS;
    }
    GxPlayer_MediaPlay(PLAYER_FOR_NORMAL, url_temp, 0, 0, NULL);
    ST_API_PRINTF("AfterSubturl: %s\n", url_temp);
    return ret;
}


static int subttrans_get_free_thread(void)
{
    int i = 0;
    for(i=0; i<SUBT_SERVER_MAX; i++)
    {
        if(st_PreInfoList[i].is_download == 0)
            return i;
    }
    return -1;
}

static void _trans_info_prepare_clear_one(int id)
{
    if(st_PreInfoList[id].param.subt_data)
    {
        GxCore_Free(st_PreInfoList[id].param.subt_data);
        st_PreInfoList[id].param.subt_data = NULL;
    }
    memset(&st_PreInfoList[id].param, 0, sizeof(GxST_TransParams_t));
    st_PreInfoList[id].thread_id = 0;
    st_PreInfoList[id].is_download = 0;
}

static void subttrans_translation_info_prepare_init(void)
{
    int i = 0;
    for(i=0; i<SUBT_SERVER_MAX; i++) {
        _trans_info_prepare_clear_one(i);
    }
}

static void subttrans_start_translation_feed(void *arg)
{
    gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
    GxST_TransGet_t *get_trans = NULL;

    GxCore_ThreadDetach();
    if(!arg) return;

    int id = *(int *)arg;
    GxCore_Free(arg);
    arg = NULL;
    if(id<0 || id>SUBT_SERVER_MAX) {
        return;
    }
    st_PreInfoList[id].is_download = 1;
    ST_API_PRINTF("==> Gx_SubtTrans_Translation[%d] Input Info  : request_id:%d, subttype=%d, service_id=%d, data_len=%d =====\n", id,
            st_PreInfoList[id].param.request_id,
            st_PreInfoList[id].param.subt_type,
            st_PreInfoList[id].param.service_id,
            st_PreInfoList[id].param.subt_len);

    ret = Gx_SubtTrans_Translation(&st_PreInfoList[id].param, &get_trans);
    if(ret == GXAPPS_SUCCESS)
    {
        g_STDrawPool.put(get_trans->pts, get_trans->trans_subt);
        ST_API_PRINTF("==> Gx_SubtTrans_Translation[%d] Success Info: request_id:%d, data: %s, pts= %f\n", id,
                get_trans->request_id,
                get_trans->trans_subt,
                get_trans->pts);
    }
    else
    {
        app_log_error("==> Gx_SubtTrans_Translation[%d] ret Error\n", id);
    }

    if(get_trans) {
        Gx_SubtTrans_Translation_Free(get_trans);
    }

    _trans_info_prepare_clear_one(id);
    st_PreInfoList[id].is_download = 0;
}

static void create_st_trans_thread(int id)
{
    int* id_ptr = malloc(sizeof(int));
    *id_ptr = id;
    GxCore_ThreadCreate("st_trans", &st_PreInfoList[id].thread_id, subttrans_start_translation_feed,
            (void*)id_ptr, 128*1024, GXOS_DEFAULT_PRIORITY);
}

static void subttrans_start_translation_prepare(AppMsg_SubtTransControl *msg, int id)
{
    if(!msg || !msg->data || (msg->data_len<=0))
        return ;

    _trans_info_prepare_clear_one(id);
    st_PreInfoList[id].param.subt_data = (uint8_t *)GxCore_Calloc(msg->data_len, sizeof(uint8_t));
    if(!st_PreInfoList[id].param.subt_data)
        return;

    if(msg->subt_type == 2)//SUBT_MODE_DVB
        st_PreInfoList[id].param.subt_type = ST_SUBT_TYPE_DVB;
    else
        st_PreInfoList[id].param.subt_type = ST_SUBT_TYPE_TTX;
    st_PreInfoList[id].param.service_id = msg->service_id;
    st_PreInfoList[id].param.request_id = ST_REQUEST_DEFAULT+id;
    memcpy(st_PreInfoList[id].param.subt_data, msg->data, msg->data_len);
    st_PreInfoList[id].param.subt_len = msg->data_len;
    strncpy(st_PreInfoList[id].param.source_lang, gx_st_api_lang_isolang(msg->lang), LANG639_STR_MAX);
    strncpy(st_PreInfoList[id].param.target_lang, gx_st_api_get_trans_lang(), LANG639_STR_MAX);
    create_st_trans_thread(id);
}

int app_subttrans_ctl_msg_proc(GxMessage *msg)
{
    AppMsg_SubtTransControl *st_msg = NULL;
    int free_thread_id = 0;
    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    st_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_SubtTransControl);
    if(!st_msg)
        return EVENT_TRANSFER_STOP;

    switch(st_msg->type)
    {
        case STMSG_DVB_SUBT_GET:
            {
                if(st_msg->data && (st_msg->data_len>0))
                {
                    if(true == app_st_check_need_show()) {
                        //app_log_debug("!!! get STMSG_DVB_SUBT_GET data_len=%d, service_id=%d\n", st_msg->data_len, st_msg->service_id);
                        free_thread_id = subttrans_get_free_thread();
                        if(free_thread_id >= 0) {
                            subttrans_start_translation_prepare(st_msg, free_thread_id);
                        }
                    }
                    GxCore_Free(st_msg->data);
                    st_msg->data = NULL;
                }
            }
            break;

        case STMSG_DVB_SUBT_DRAW:
            break;

        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

#endif


