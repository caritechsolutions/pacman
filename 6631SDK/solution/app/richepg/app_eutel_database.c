#include "app.h"
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "richepg_block_mem.h"
#include "richepg_common.h"
#include "app_eutel_database.h"
#include <assert.h>
#include "richepg_json_api.h"
#include "app_eutel_common.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define RICHEPG_TS_PATH_DF  "/dvb/theme/richepg_ts.json"

#define PM_UPDATE_OPTIMIZE  1 // 设为1时更新数据库时先不全部删除TP，为了不删除PVR的预约
#if PM_UPDATE_OPTIMIZE
#define PM_UPDATE_USE_CHID  1 // 是否使用ECL的channel id作为唯一可信的区分节目的参数
#else
#define PM_UPDATE_USE_CHID  0
#endif

/******** preset sat and ts manage ********/
#define RICHEPG_TS_IMPORT_PATH  "/home/gx/import_ts.json"
#ifdef LINUX_OS
#define RICHEPG_TS_TEMP_PATH    "/tmp/import_ts.json"
#else
#define RICHEPG_TS_TEMP_PATH    "/mnt/import_ts.json"
#endif

int app_eutel_import_ts(const char *src)
{
    if (richepg_copy_file_to_file(src, RICHEPG_TS_IMPORT_PATH, 8192) > 0)
        return 0;
    unlink(RICHEPG_TS_IMPORT_PATH);
    return -1;
}

int app_eutel_ts_mgr_parse(void)
{
    int ret = -1;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();

    if (GxCore_FileExists(RICHEPG_TS_IMPORT_PATH) == GXCORE_FILE_EXIST)
    {
        if (richepg_copy_file_to_file(RICHEPG_TS_IMPORT_PATH, RICHEPG_TS_TEMP_PATH, 8192) > 0)
        {
            unlink(RICHEPG_TS_IMPORT_PATH);
            mgr->default_ts_path = RICHEPG_TS_TEMP_PATH;
            ret = richepg_ts_mgr_parse();
            unlink(RICHEPG_TS_TEMP_PATH);
            mgr->default_ts_path = RICHEPG_TS_PATH_DF;
            if (ret < 0)
            {
                richepg_ts_mgr_release();
                ret = richepg_ts_mgr_parse();
            }
        }
        else
        {
            unlink(RICHEPG_TS_IMPORT_PATH);
            ret = richepg_ts_mgr_parse();
        }
    }
    else
    {
        ret = richepg_ts_mgr_parse();
    }

    return ret;
}

static int _check_add_sat(GxBusPmDataSat *psat)
{
    GxBusPmDataSat sat;
    int sat_num = 0;
    int i = 0;

    sat_num = GxBus_PmSatNumGet();
    for (i = 0; i < sat_num; i++) {
        GxBus_PmSatsGetByPos(i, 1, &sat);
        if (sat.type == GXBUS_PM_SAT_S && sat.sat_s.reserved == 1 &&
				strcmp((char*)sat.sat_s.sat_name, (char*)psat->sat_s.sat_name) == 0) {
            psat->id = sat.id;
            GxBus_PmSatModify(psat);
            goto end;
        }
    }
    psat->sat_s.reserved = 1;
    if (GxBus_PmSatAdd(psat) == GXCORE_ERROR) {
        EUTEL_ERR("Add sat failed!\n");
        return -1;
    }

end:
    GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    return 0;
}

static int _add_sat_cb(void *json, void **sat_data)
{
    cJSON *tmp1 = (cJSON *)json, *tmp2 = NULL;
    GxBusPmDataSat *sat = NULL;

    if (*sat_data) {
        sat = *sat_data;
    } else {
        if ((sat = GxCore_Calloc(1, sizeof(GxBusPmDataSat))) == NULL) {
            EUTEL_ERR("malloc failed!\n");
            return -1;
        }
        *sat_data = sat;
    }

    sat->type = GXBUS_PM_SAT_S;
#if UNICABLE_SUPPORT
    sat->sat_s.unicable_para.lnb_fre_index = 0x20;
#endif
    tmp2 = cJSON_GetObjectItem(tmp1, "Tuner");
    sat->tuner = tmp2->valueint;
    tmp2 = cJSON_GetObjectItem(tmp1, "LNB1");
    sat->sat_s.lnb1 = tmp2->valueint;
    tmp2 = cJSON_GetObjectItem(tmp1, "LNB2");
    sat->sat_s.lnb2 = tmp2->valueint;
    tmp2 = cJSON_GetObjectItem(tmp1, "Sat12v");
    sat->sat_s.switch_12V = (tmp2->valueint == 0) ? GXBUS_PM_SAT_12V_OFF : GXBUS_PM_SAT_12V_ON;
    if (sat->sat_s.lnb1 > 9000 && sat->sat_s.lnb2 > 9000) {
        sat->sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
    } else {
        tmp2 = cJSON_GetObjectItem(tmp1, "Sat22k");
        sat->sat_s.switch_22K = (tmp2->valueint == 0) ? GXBUS_PM_SAT_22K_OFF : GXBUS_PM_SAT_22K_ON;
    }
    tmp2 = cJSON_GetObjectItem(tmp1, "Diseqc10");
    sat->sat_s.diseqc10 = tmp2->valueint;
    sat->sat_s.diseqc11 = 0;
    tmp2 = cJSON_GetObjectItem(tmp1, "Diseqc12Pos");
    sat->sat_s.diseqc12_pos = tmp2->valueint;
    sat->sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
    sat->sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
    tmp2 = cJSON_GetObjectItem(tmp1, "Longitude");
    sat->sat_s.longitude = tmp2->valueint;
    tmp2 = cJSON_GetObjectItem(tmp1, "Direction");
    sat->sat_s.longitude_direct = (tmp2->valueint) ? GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST : GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;
    tmp2 = cJSON_GetObjectItem(tmp1, "Name");
    strncpy((char*)sat->sat_s.sat_name, tmp2->valuestring, MAX_SAT_NAME-1);
    if (_check_add_sat(sat) < 0)
        return -1;

    return sat->id;
}

static int _get_sat_cb(int sat_id, void **sat_data)
{
    GxBusPmDataSat *sat = NULL;

    if (*sat_data) {
        sat = *sat_data;
    } else {
        if ((sat = GxCore_Calloc(1, sizeof(GxBusPmDataSat))) == NULL) {
            EUTEL_ERR("malloc failed!\n");
            return -1;
        }
        *sat_data = sat;
    }

    if (GxBus_PmSatGetById(sat_id, sat) == GXCORE_ERROR) {
        EUTEL_ERR("get sat(%d) failed!\n", sat_id);
        return -1;
    }

    return 0;
}

static int _free_sat_cb(void **sat_data)
{
    if (*sat_data) {
        GxCore_Free(*sat_data);
        *sat_data = NULL;
    }
    return 0;
}

int app_eutel_ts_mgr_init(void)
{
    RichepgTsMgrCb cb;
    const char *ignore_sat_name = NULL;

    cb.add_sat = _add_sat_cb;
    cb.get_sat = _get_sat_cb;
    cb.free_sat = _free_sat_cb;
    if (!app_richepg_certification_access_get())
        ignore_sat_name = "Certification";

    return richepg_ts_mgr_init(cb, RICHEPG_TS_PATH_DF, ignore_sat_name);
}

bool app_eutel_same_ts_check(const RichepgTsTP *tp1, const RichepgTsTP *tp2)
{
    uint32_t interval_fre = 5;

    if(tp2->symbol_rate > 20000)
        interval_fre = (tp2->symbol_rate+2000)/4000;
    else if(tp2->symbol_rate < 10000)
        interval_fre = (tp2->symbol_rate+1000)/2000;

    if ((tp1->polar == tp2->polar)
            && (tp1->frequency - interval_fre <= tp2->frequency)
            && (tp1->frequency + interval_fre >= tp2->frequency)
            && (tp1->symbol_rate - 100 <= tp2->symbol_rate)
            && (tp1->symbol_rate + 100 >= tp2->symbol_rate))
        return true;
    return false;
}

bool app_eutel_trusted_tp_check(GxBusPmDataTP *tp)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsTP tmp_tp = {0};
    int i = 0, j = 0;

    tmp_tp.frequency = tp->frequency;
    tmp_tp.symbol_rate = tp->tp_s.symbol_rate;
    tmp_tp.polar = tp->tp_s.polar;

    for (i = 0; i < mgr->sat_num; i++) {
        if (tp->sat_id == mgr->sat_array[i].sat_id) {
            for (j = 0; j < mgr->sat_array[i].tp_num; j++) {
                if (app_eutel_same_ts_check(&mgr->sat_array[i].tp_array[j], &tmp_tp))
                    return true;
            }
            return false;
        }
    }
    return false;
}

bool app_eutel_sat_type_check(int sat_id)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    int i = 0;
    for (i = 0; i < mgr->sat_num; i++) {
        if (sat_id == mgr->sat_array[i].sat_id) {
            return true;
        }
    }
    return false;
}

int app_eutel_ts_set_lineup(int id)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    mgr->sat_array[mgr->sat_sel].lineup_id = id;
    return 0;
}

int app_eutel_sat_sel_set(int sel, bool change_store)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    if (sel < mgr->sat_num) {
        mgr->sat_sel = sel;
        if (change_store)
            richepg_store_setsel(sel);
        return 0;
    }
    return -1;
}

int app_eutel_sat_id_get(int sel)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    return mgr->sat_array[sel].sat_id;
}

int app_eutel_cur_sat_id_get(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    return mgr->sat_array[mgr->sat_sel].sat_id;
}

GxBusPmDataSat *app_eutel_sat_data_get(int sel)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    return (GxBusPmDataSat *)mgr->sat_array[sel].sat_data;
}

GxBusPmDataSat *app_eutel_cur_sat_data_get(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    return (GxBusPmDataSat *)mgr->sat_array[mgr->sat_sel].sat_data;
}

void app_eutel_sat_save_params(int sel,GxBusPmDataSat *p_sat)
{
    GxBusPmDataSat *psat = NULL;

    psat = app_eutel_sat_data_get(sel);

    if ((p_sat->sat_s.lnb_power == psat->sat_s.lnb_power)
                && (p_sat->sat_s.lnb1 == psat->sat_s.lnb1)
                && (p_sat->sat_s.lnb2 == psat->sat_s.lnb2)
                && (p_sat->sat_s.switch_22K == psat->sat_s.switch_22K)
                && (p_sat->sat_s.diseqc10 == psat->sat_s.diseqc10)
                && (p_sat->sat_s.diseqc11 == psat->sat_s.diseqc11)) {
        return ;
    }

    psat->sat_s.lnb_power = p_sat->sat_s.lnb_power ;
    psat->sat_s.lnb1 = p_sat->sat_s.lnb1 ;
    psat->sat_s.lnb2 = p_sat->sat_s.lnb2  ;
    psat->sat_s.switch_22K = p_sat->sat_s.switch_22K ;
    psat->sat_s.diseqc10 = p_sat->sat_s.diseqc10 ;
    psat->sat_s.diseqc11 = p_sat->sat_s.diseqc11 ;

    GxBus_PmSatModify(psat);
    GxBus_PmSync(GXBUS_PM_SYNC_SAT);
}

int app_eutel_get_tp_by_sel(int sel,unsigned int *fre,unsigned int *symbol,unsigned int *polar)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsTP *tp = NULL ;

    if ( (sel < 0 )|| (sel >= mgr->sat_num) )
    {
        EUTEL_ERR("get eutel sat sel empty \n");
        return -1;
    }

    tp = &mgr->sat_array[sel].tp_array[0];

    if ( fre )
        *fre = tp->frequency ;
    if ( symbol )
        *symbol = tp->symbol_rate ;
    if ( polar )
    {
        if(RICHEPG_TS_POLAR_V == tp->polar)
            *polar = GXBUS_PM_TP_POLAR_V ;
        else
            *polar = GXBUS_PM_TP_POLAR_H ;
    }
    return 0 ;
}

int app_eutel_get_valid_sat_id(int sel)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    if (mgr == NULL || mgr->sat_array == NULL )
        return 0 ;
    if ( sel < 0 || sel >= mgr->sat_num )
        return 0 ;
    if (mgr->sat_array[sel].lineup_id)
        return mgr->sat_array[sel].sat_id;

    return 0 ;
}

/******** prog list manage ********/

static int app_eutel_proglist_tp_release(void)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    int i = 0;
    if (ctrl->tp_array) {
        for (i = 0; i < ctrl->tp_num; i++) {
            if (ctrl->tp_array[i].ch_array)
                GxCore_Free(ctrl->tp_array[i].ch_array);
        }
        GxCore_Free(ctrl->tp_array);
    }
    ctrl->tp_num = 0;
    ctrl->tp_array = NULL;
    return 0;
}

static int _eutel_channel_id_get_cb(int i)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    return ctrl->prog_array[i].id;
}

int app_eutel_proglist_skip_set(bool skip_set)
{
    GxBusPmViewInfo view_info = {0};
    EUTEL_SINFO("proglist skip set = %d \n",skip_set);
    if (skip_set == false) {
        s_eutel_ch_ctrl.cur_stream_type = 0;

        memcpy(&view_info,&(g_AppPlayOps.normal_play.view_info), sizeof(GxBusPmViewInfo));
        view_info.stream_type = GXBUS_PM_PROG_TV;
        view_info.group_mode = GROUP_MODE_SAT;
        view_info.sat_id = app_eutel_cur_sat_id_get();
        //GxBus_PmViewInfoModify(&view_info);
        g_AppPlayOps.play_list_create(&view_info);
    }
    else{
        memcpy(&view_info,&(g_AppPlayOps.normal_play.view_info), sizeof(GxBusPmViewInfo));
        if ( (g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_SAT ) &&
		     (app_eutel_sat_type_check(g_AppPlayOps.normal_play.view_info.sat_id)) )
        {
            view_info.group_mode = GROUP_MODE_ALL;
        }
        g_AppPlayOps.play_list_create(&view_info);
    }
    return 0 ;
}

static inline int _get_tp_index(int onid, int tsid)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    int i = 0;
    for (i = 0; i < ctrl->tp_num; i++) {
        if (onid == ctrl->tp_array[i].onid && tsid == ctrl->tp_array[i].tsid)
            return i;
    }
    return -1;
}

#if PM_UPDATE_USE_CHID
static inline int _pm_chid_get(GxBusPmDataProg *prog)
{
    return prog->nvod_service_id;
}

static inline void _pm_chid_set(GxBusPmDataProg *prog, int chid)
{
    prog->nvod_service_id = chid;
}

static inline int _get_tp_prog_id(int chid, RichepgProgListTP *tp)
{
    int i = 0;
    for (i = 0; i < tp->ch_num; i++) {
        if (chid == tp->ch_array[i].chid)
            return tp->ch_array[i].progid;
    }
    return -1;
}
#else
static inline int _get_tp_prog_id(int sid, RichepgProgListTP *tp)
{
    int i = 0;
    for (i = 0; i < tp->ch_num; i++) {
        if (sid == tp->ch_array[i].sid)
            return tp->ch_array[i].progid;
    }
    return -1;
}
#endif

static inline int _get_elld_index(struct eutelsat_lineup_data *elld, int id)
{
    int i = 0;
    for (i = 0; i < elld->channel_num; i++) {
        if (id == elld->channels[i].id)
            return i;
    }
    return -1;
}

static inline int _get_dvb_tech(struct eutelsat_channel *tech)
{
    int i = 0;
    for (i = 0; i < tech->tech_num; i++) {
        if (EUTELSAT_DELIVERY_DVB == tech->techs[i].delivery)
            return i;
    }
    return -1;
}

char *app_eutel_proglist_prog_name_get(int num, RichepgProgNameItem *names, const char *lang)
{
    int i = 0;

    // first current menu language
    for (i = 0; i < num; i++) {
        if (names[i].lang && strncmp(names[i].lang, lang, 3) == 0) {
            return names[i].name;
        }
    }
    // second default ext language
    for (i = 0; i < num; i++) {
        if (names[i].is_default) {
            return names[i].name;
        }
    }

    return names[0].name;
}

static int app_eutel_collect_prog_info(int (*collect)(int, char *, char *), char *prog_name,
        struct eutelsat_lineup_channel *elld_ch, struct eutelsat_configuration_file *ecf)
{
    int ret = -1;
    int i = 0, j = 0, pos = 0, lcn = 0;
    char genres_name[256] = {0};
    extern void eutel_strlink(char *dst, char *src, int dst_size);

    if (elld_ch != NULL && elld_ch->lcn_num > 0) {
        lcn = elld_ch->lcns[0];
    }
    if ((elld_ch != NULL && elld_ch->genres_num > 0)
            && (ecf != NULL && ecf->channel_num > 0)) {
        for (j = 0; j < elld_ch->genres_num; j++) {
            for (i = 0; i < ecf->channel_num; i++) {
                if (elld_ch->genres[j] == ecf->channel[i].id) {
                    pos = richepg_get_locale_index(ecf->channel[i].locales_num, ecf->channel[i].locales);
                    if (pos >= 0 && ecf->channel[i].locales[pos].name != NULL) {
                        eutel_strlink(genres_name, ecf->channel[i].locales[pos].name, sizeof(genres_name));
                    }
                    break;
                }
            }
        }
        ret = collect(lcn, prog_name, genres_name);
    } else {
        ret = collect(lcn, prog_name, NULL);
    }

    return ret;
}

static void _set_prog_params(GxBusPmDataProg *prog, GxBusPmDataProg *prog_bak)
{
    if (prog_bak) {
        prog->favorite_flag = prog_bak->favorite_flag;
        prog->lock_flag = prog_bak->lock_flag;

        prog->audio_volume = prog_bak->audio_volume;
        prog->audio_mode = prog_bak->audio_mode;
        prog->logic_valid = prog_bak->logic_valid;
        prog->bouquet_id = prog_bak->bouquet_id;
        prog->pmt_version = prog_bak->pmt_version;
        prog->audio_level = prog_bak->audio_level;
    } else {
        prog->audio_volume = 50;
        prog->audio_mode = GXBUS_PM_PROG_AUDIO_MODE_STEREO;
        prog->logic_valid = 0;
        prog->bouquet_id = 0;    //等待si解析bat表
        prog->pmt_version = 0;   //需要监控的话用初始0版本也是可以的
        prog->audio_level = GXBUS_PM_PROG_AUDIO_LECEL_MID;
    }
}

static int app_eutel_proglist_prog_add(struct eutelsat_channel *ecl_ch, struct eutelsat_techchannel *tech_ch,
        struct eutelsat_lineup_channel *elld_ch, struct eutelsat_configuration_file *ecf,
        int sat_id, RichepgProgListTP *tp)
{
    GxBusPmDataProg prog = {0};
    int name_index = 0;
    char *prog_name = NULL;
#if PM_UPDATE_OPTIMIZE
#if PM_UPDATE_USE_CHID
    int prog_id = _get_tp_prog_id(ecl_ch->id, tp);
#else
    int prog_id = _get_tp_prog_id(tech_ch->trans.dvb.sid, tp);
#endif
    if (prog_id > 0) {
        return prog_id;
    }
#endif

    prog.sat_id = sat_id;
    prog.tp_id = tp->tpid;
    prog.service_id = tech_ch->trans.dvb.sid;
    prog.original_id = tech_ch->trans.dvb.onid;
    prog.ts_id = tech_ch->trans.dvb.tsid;
    prog.scramble_flag = (ecl_ch->scrambled) ? GXBUS_PM_PROG_BOOL_ENABLE : GXBUS_PM_PROG_BOOL_DISABLE;
    name_index = richepg_get_locale_index(ecl_ch->locales_num, ecl_ch->locales);
    if (name_index >= 0 && ecl_ch->locales[name_index].name != NULL) {
        prog_name = ecl_ch->locales[name_index].name;
    } else {
        prog_name = "No name";
    }
    strncpy((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1);
#if PM_UPDATE_USE_CHID
    _pm_chid_set(&prog, ecl_ch->id);
#endif
    _set_prog_params(&prog, NULL);
    if(GxBus_PmProgAdd(&prog) == GXCORE_ERROR) {
        EUTEL_ERR("Add prog failed!\n");
        return -1;
    }

    if (app_eutel_maint_collect_check()) {
        app_eutel_collect_prog_info(app_eutel_collect_add_item_ch_add, prog_name, elld_ch, ecf);
    }

    return prog.id;
}

static int app_eutel_collect_channel_genres(struct eutelsat_channel_list *ecl, struct eutelsat_lineup_data *elld,
        struct eutelsat_configuration_file *ecf)
{
    int i = 0, j = 0, k = 0, count = 0, pos = 0;

    if (!app_eutel_install_collect_check())
        return -1;

    if (ecf != NULL && ecf->channel_num > 0) {
        for (i = 0; i < ecf->channel_num; i++) {
            count = 0;
            for (j = 0; j < elld->channel_num; j++) {
                for (k = 0; k < elld->channels[j].genres_num; k++) {
                    if (elld->channels[j].genres[k] == ecf->channel[i].id) {
                        ++count;
                        break;
                    }
                }
            }
            if (count > 0) {
                pos = richepg_get_locale_index(ecf->channel[i].locales_num, ecf->channel[i].locales);
                if (pos >= 0 && ecf->channel[i].locales[pos].name != NULL) {
                    app_eutel_collect_add_item_genre_add(count, ecf->channel[i].locales[pos].name);
                }
            }
        }
    }
    app_eutel_collect_add_item_genre_add(ecl->channels_num, NULL);

    return 0;
}

static char *_get_locales_def_name(struct eutelsat_channel *ecl_ch)
{
    int i = 0;

    if(!ecl_ch || !ecl_ch->locales || !ecl_ch->locales_num)
        return "null";
    for(i = 0; i < ecl_ch->locales_num; i++)
    {
        if(ecl_ch->locales[i].lang && ecl_ch->locales[i].name && 0 == strcmp(ecl_ch->locales[i].lang, RICHEPG_LOCALE_LANGUAGE_DEF))
            return ecl_ch->locales[i].name;
    }

    return "null";
}

static int app_eutel_proglist_prog_process(struct eutelsat_channel_list *ecl, struct eutelsat_lineup_data *elld,
        struct eutelsat_configuration_file *ecf, struct eutelsat_additional_resources *ear, int add_prog)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    struct eutelsat_channel *ecl_ch = NULL;
    struct eutelsat_lineup_channel *elld_ch = NULL;
    struct eutelsat_techchannel *tech_ch = NULL;
    int tech_index = 0, tp_index = 0, prog_index = 0, logo_index = 0;
    int sat_id = 0, prog_id = 0;
    int count = 0;
    int i = 0, j = 0;
    char *ear_path = NULL;

    richepg_proglist_release();
    ctrl->prog_array = block_mem_mgr_alloc_data(elld->channel_num * sizeof(RichepgProgListProg), &ctrl->struct_mgr);
    if (ctrl->prog_array == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    sat_id = app_eutel_cur_sat_id_get();

    for (i = 0; i < ecl->channels_num; i++) {
        ecl_ch = &ecl->channels[i];

        tech_index = _get_dvb_tech(ecl_ch);
        if (tech_index < 0)
            continue;
        tech_ch = &ecl_ch->techs[tech_index];

        tp_index = _get_tp_index(tech_ch->trans.dvb.onid, tech_ch->trans.dvb.tsid);
        if (tp_index < 0) {
            EUTEL_ERR("channel %d %s tp not find! onid=%d, tsid=%d\n", ecl_ch->id, _get_locales_def_name(ecl_ch), tech_ch->trans.dvb.onid, tech_ch->trans.dvb.tsid);
            continue;
        }

        prog_index = _get_elld_index(elld, ecl_ch->id);
        if (prog_index < 0) {
            elld_ch = NULL;
#if FAST_SWITCH_PM_SUPPORT
            if (add_prog) {
                prog_id = app_eutel_proglist_prog_add(ecl_ch, tech_ch, elld_ch, ecf, sat_id, &ctrl->tp_array[tp_index]);
            }
#endif
            continue;
        } else {
            elld_ch = &elld->channels[prog_index];
            if (add_prog) {
                prog_id = app_eutel_proglist_prog_add(ecl_ch, tech_ch, elld_ch, ecf, sat_id, &ctrl->tp_array[tp_index]);
            } else {
#if PM_UPDATE_USE_CHID
                prog_id = _get_tp_prog_id(ecl_ch->id, &ctrl->tp_array[tp_index]);
#else
                prog_id = _get_tp_prog_id(tech_ch->trans.dvb.sid, &ctrl->tp_array[tp_index]);
#endif
            }
            if (prog_id <= 0)
                continue;
        }

        logo_index = richepg_get_logo_index(ecl_ch->logos_num, ecl_ch->logos);
        if (logo_index != -1 && ecl_ch->logos[logo_index].path) {
            ctrl->prog_array[count].prog_logo = block_mem_mgr_dup_str(ecl_ch->logos[logo_index].path, &ctrl->string_mgr);
            if (ctrl->prog_array[count].prog_logo == NULL) goto err;
        }

        if (ear) {
            ear_path = richepg_get_ear_channel_logo(ecl_ch->id);
            if (ear_path) {
                ctrl->prog_array[count].ear_logo = block_mem_mgr_dup_str(ear_path, &ctrl->ear_mgr);
            } else {
                ctrl->prog_array[count].ear_logo = NULL;
            }
        }

        if (elld_ch->lcn_num) {
            ctrl->prog_array[count].lcns_num = elld_ch->lcn_num;
            ctrl->prog_array[count].lcns_array = block_mem_mgr_alloc_data(ctrl->prog_array[count].lcns_num * sizeof(int), &ctrl->struct_mgr);
            if (ctrl->prog_array[count].lcns_array == NULL) goto err;
            for (j = 0; j < ctrl->prog_array[count].lcns_num; j++) {
                ctrl->prog_array[count].lcns_array[j] = elld_ch->lcns[j];
            }
        }

        if (elld_ch->genres_num) {
            ctrl->prog_array[count].genres_num = elld_ch->genres_num;
            ctrl->prog_array[count].genres_array = block_mem_mgr_alloc_data(ctrl->prog_array[count].genres_num * sizeof(int), &ctrl->struct_mgr);
            if (ctrl->prog_array[count].genres_array == NULL) goto err;
            for (j = 0; j < ctrl->prog_array[count].genres_num; j++) {
                ctrl->prog_array[count].genres_array[j] = elld_ch->genres[j];
            }
        }

        if (tech_ch->services_num) {
            ctrl->prog_array[count].types_num = tech_ch->services_num;
            ctrl->prog_array[count].types_array = block_mem_mgr_alloc_data(ctrl->prog_array[count].types_num * sizeof(int), &ctrl->struct_mgr);
            if (ctrl->prog_array[count].types_array == NULL) goto err;
            for (j = 0; j < ctrl->prog_array[count].types_num; j++) {
                ctrl->prog_array[count].types_array[j] = tech_ch->services[j];
            }
        }

        if (ecl_ch->locales_num) {
            ctrl->prog_array[count].names_num = ecl_ch->locales_num;
            ctrl->prog_array[count].names_array = block_mem_mgr_alloc_data(ctrl->prog_array[count].names_num * sizeof(RichepgProgNameItem), &ctrl->struct_mgr);
            if (ctrl->prog_array[count].names_array == NULL) goto err;
            for (j = 0; j < ctrl->prog_array[count].names_num; j++) {
                ctrl->prog_array[count].names_array[j].is_default = ecl_ch->locales[j].is_default;
                if (ecl_ch->locales[j].lang) {
                    ctrl->prog_array[count].names_array[j].lang = block_mem_mgr_dup_str(ecl_ch->locales[j].lang, &ctrl->string_mgr);
                    if (ctrl->prog_array[count].names_array[j].lang == NULL) goto err;
                }
                if (ecl_ch->locales[j].name) {
                    ctrl->prog_array[count].names_array[j].name = block_mem_mgr_dup_str(ecl_ch->locales[j].name, &ctrl->string_mgr);
                    if (ctrl->prog_array[count].names_array[j].name == NULL) goto err;
                }
            }
        }

        ctrl->prog_array[count].id = ecl_ch->id;
        ctrl->prog_array[count].prog_id = prog_id;
        ++count;
    }

    GxBus_PmSync(GXBUS_PM_SYNC_PROG);
    ctrl->prog_num = count;
    app_eutel_proglist_tp_release();
    block_mem_mgr_adjust_size(&ctrl->struct_mgr);
    block_mem_mgr_adjust_size(&ctrl->string_mgr);
    block_mem_mgr_adjust_size(&ctrl->ear_mgr);
    richepg_epg_id_array_update(ctrl->prog_num, _eutel_channel_id_get_cb);
    return 0;

err:
    GxBus_PmSync(GXBUS_PM_SYNC_PROG);
    app_eutel_proglist_tp_release();
    richepg_proglist_release();
    return -1;
}

static int app_eutel_proglist_tp_delete(void)
{
    int i = 0;
    int tp_num = 0;
    int sat_id = 0;
    uint32_t *id_array = NULL;
    GxBusPmDataTP tp = {0};

    sat_id = app_eutel_cur_sat_id_get();
    tp_num = GxBus_PmTpNumGetBySat(sat_id);
    if (tp_num < 0)
        return -1;
    if (tp_num == 0)
        return 0;
    if ((id_array = GxCore_Calloc(tp_num, sizeof(uint32_t))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        return -1;
    }

    for (i = 0; i < tp_num; i++) {
        GxBus_PmTpGetByPosInSat(i, 1, &tp);
        id_array[i] = tp.id;
    }
    GxBus_PmTpDelete(id_array, tp_num);
    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    GxCore_Free(id_array);

    return 0;
}

static int app_eutel_proglist_tp_process_pm(struct eutelsat_channel_list *ecl)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    GxBusPmViewInfo view_info = {0};
    GxBusPmViewInfo view_info_bak = {0};
    GxBusPmDataTP tp = {0};
    GxBusPmDataProg prog = {0};
    int sat_id = 0;
    int tp_num = 0;
    int prog_num = 0;
    int i = 0, j = 0;
#if PM_UPDATE_USE_CHID
    uint32_t *del_id_array = NULL;
    uint32_t del_id_num = 0;
#endif

    sat_id = app_eutel_cur_sat_id_get();
    tp_num = GxBus_PmTpNumGetBySat(sat_id);
    if (tp_num == 0) {
        EUTEL_ERR("No tp!\n");
        return 0;
    }
    if ((ctrl->tp_array = GxCore_Calloc(tp_num, sizeof(RichepgProgListTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        return -1;
    }

    for (j = 0; j < tp_num; j++) {
        GxBus_PmTpGetByPosInSat(j, 1, &tp);
        ctrl->tp_array[j].tpid = tp.id;
    }

    GxBus_PmViewInfoGet(&view_info);
    memcpy(&view_info_bak, &view_info, sizeof(GxBusPmViewInfo));
    view_info.stream_type = GXBUS_PM_PROG_ALL;
    view_info.group_mode = GROUP_MODE_SAT;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    view_info.sat_id = sat_id;
    GxBus_PmViewInfoModify(&view_info);
    prog_num = GxBus_PmProgNumGet();
    if (prog_num <= 0) {
        EUTEL_ERR("No prog!\n");
        goto end;
    }

#if PM_UPDATE_USE_CHID
    if (ecl != NULL && (del_id_array = GxCore_Calloc(prog_num, sizeof(uint32_t))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
#endif

    for (i = 0; i < prog_num; i++) {
        GxBus_PmProgGetByPos(i, 1, &prog);
#if PM_UPDATE_USE_CHID
        if (ecl) {
            for (j = 0; j < ecl->channels_num; j++) {
                if (ecl->channels[j].id == _pm_chid_get(&prog)) {
                    break;
                }
            }
            if (j == ecl->channels_num) {
                del_id_array[del_id_num++] = prog.id;
                EUTEL_SINFO("Process: \033[32mProg(progid=%d:chid=%d %s) is deleted\033[0m\n", prog.id, _pm_chid_get(&prog), (char*)prog.prog_name);
                continue;
            }
        }
#endif
        for (j = 0; j < tp_num; j++) {
            if (ctrl->tp_array[j].tpid == prog.tp_id) {
                ctrl->tp_array[j].onid = prog.original_id;
                ctrl->tp_array[j].tsid = prog.ts_id;
                ++ctrl->tp_array[j].ch_num;
                break;
            }
        }
    }

#if PM_UPDATE_USE_CHID
    if (del_id_num) {
        GxBus_PmProgDelete(del_id_array, del_id_num);
        GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        GxCore_Free(del_id_array);
        del_id_array = NULL;

        prog_num = GxBus_PmProgNumGet();
        if (prog_num <= 0) {
            EUTEL_ERR("No prog!\n");
            goto end;
        }
   } else {
       if (del_id_array) {
           GxCore_Free(del_id_array);
           del_id_array = NULL;
       }
   }
#endif

    for (j = 0; j < tp_num; j++) {
        if (ctrl->tp_array[j].ch_num > 0) {
            if ((ctrl->tp_array[j].ch_array = GxCore_Calloc(ctrl->tp_array[j].ch_num, sizeof(RichepgProgListCH))) == NULL) {
                EUTEL_ERR("malloc failed!\n");
                for (i = 0; i < j; i++) {
                    if (ctrl->tp_array[i].ch_array)
                        GxCore_Free(ctrl->tp_array[i].ch_array);
                }
                goto err;
            }
            ctrl->tp_array[j].ch_num = 0;
        }
    }

    for (i = 0; i < prog_num; i++) {
        GxBus_PmProgGetByPos(i, 1, &prog);
        for (j = 0; j < tp_num; j++) {
            if (ctrl->tp_array[j].tpid == prog.tp_id) {
                ctrl->tp_array[j].onid = prog.original_id;
                ctrl->tp_array[j].tsid = prog.ts_id;
                ctrl->tp_array[j].ch_array[ctrl->tp_array[j].ch_num].sid = prog.service_id;
                ctrl->tp_array[j].ch_array[ctrl->tp_array[j].ch_num].progid = prog.id;
#if PM_UPDATE_USE_CHID
                ctrl->tp_array[j].ch_array[ctrl->tp_array[j].ch_num].chid = _pm_chid_get(&prog);
#endif
                ++ctrl->tp_array[j].ch_num;
                break;
            }
        }
    }

end:
    ctrl->tp_num = tp_num;
    GxBus_PmViewInfoModify(&view_info_bak);
    return 0;
err:
    GxBus_PmViewInfoModify(&view_info_bak);
    GxCore_Free(ctrl->tp_array);
    ctrl->tp_array = NULL;
    return -1;
}

void app_eutel_check_ecl_delivery(struct eutelsat_channel_list *ecl)
{
    struct eutelsat_channel *ecl_ch1 = NULL, *ecl_ch2 = NULL;
    struct eutelsat_techchannel *tech_ch1 = NULL, *tech_ch2 = NULL;
    int tech_index1 = 0, tech_index2 = 0;
    int i = 0, j = 0;

    if (!ecl) return;

    for (i = 0; i < ecl->channels_num; i++) {
        ecl_ch1 = &ecl->channels[i];
        tech_index1 = _get_dvb_tech(ecl_ch1);
        if (tech_index1 < 0)
            continue;
        tech_ch1 = &ecl_ch1->techs[tech_index1];

        for (j = 0; j < i; j++) {
            ecl_ch2 = &ecl->channels[j];
            tech_index2 = _get_dvb_tech(ecl_ch2);
            if (tech_index2 < 0)
                continue;
            tech_ch2 = &ecl_ch2->techs[tech_index2];
            if (tech_ch1->trans.dvb.sid == tech_ch2->trans.dvb.sid
                    && tech_ch1->trans.dvb.onid == tech_ch2->trans.dvb.onid
                    && tech_ch1->trans.dvb.tsid == tech_ch2->trans.dvb.tsid) {
                EUTEL_SINFO("\033[31merr: %5d|%5d same delivery: onid=%5d tsid=%5d sid=%5d\033[0m\n", ecl_ch1->id, ecl_ch2->id,
                        tech_ch1->trans.dvb.onid, tech_ch1->trans.dvb.tsid, tech_ch1->trans.dvb.sid);
            }
        }
    }
}

#if PM_UPDATE_OPTIMIZE

#if PM_UPDATE_USE_CHID
static int app_eutel_proglist_tp_process_nit(struct richepg_homets *homets, struct eutelsat_channel_list *ecl,
        struct eutelsat_lineup_data *elld, struct eutelsat_configuration_file *ecf)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    struct eutelsat_channel *ecl_ch = NULL;
    struct eutelsat_techchannel *tech_ch = NULL;
    struct richepg_nit_ts *ts = NULL;
    GxBusPmDataTP tp = {0};
    uint32_t prog_id = 0;
    uint32_t tp_id = 0;
    uint32_t swap_tp_id = 0;
    int sat_id = 0;
    int count = 0;
    int index = 0;
    int tech_index = 0;
    int i = 0, j = 0, m = 0, n = 0;
    bool get_flag = false;
    GxBusPmDataProg prog = {0}, prog_bak = {0};
    GxBusPmDataProgSwitch scramble_flag;
    int name_index = 0;
    char *prog_name = NULL;
    int ret = -1;

    int tp_num = 0;
    RichepgProgListTP *tp_array = NULL;
    RichepgTsTP *ts_array = NULL;
    char *pm_tp_flag = NULL;
    int tmp_num = 0;
    RichepgProgListTP *tmp_array = NULL;

    app_eutel_check_ecl_delivery(ecl);
    sat_id = app_eutel_cur_sat_id_get();

    if (app_eutel_proglist_tp_process_pm(ecl) < 0) {
        EUTEL_ERR("app_eutel_proglist_tp_process_pm failed!\n");
        return -1;
    }
    if (ctrl->tp_num > 0) {
        memset(&tp, 0, sizeof(GxBusPmDataTP));
        tp.sat_id = sat_id;
        if (GxBus_PmTpAdd(&tp) == GXCORE_ERROR) {
            EUTEL_ERR("add swap tp failed!\n");
            goto err;
        }
        swap_tp_id = tp.id;
    }
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            count += homets->nit[i].ts_count;
        }
    }
    tp_num = count;
    if (tp_num == 0) {
        EUTEL_ERR("no tp!\n");
        goto err;
    }
    if ((tp_array = GxCore_Calloc(tp_num, sizeof(RichepgProgListTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    if ((ts_array = GxCore_Calloc(tp_num, sizeof(RichepgTsTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    if (ctrl->tp_num > 0 && (pm_tp_flag = GxCore_Calloc(ctrl->tp_num, sizeof(char))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }

    count = 0;
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            for (j = 0; j < homets->nit[i].ts_count; j++) {
                ts = &homets->nit[i].ts[j];
                ts_array[count].frequency = ts->frequency;
                ts_array[count].symbol_rate = ts->symbol_rate;
                if(0 == ts->polar || 2 == ts->polar)
                    ts_array[count].polar = RICHEPG_TS_POLAR_H;
                else if(1 == ts->polar || 3 == ts->polar)
                    ts_array[count].polar = RICHEPG_TS_POLAR_V;
                else
                    ts_array[count].polar = RICHEPG_TS_POLAR_H;
                tp_array[count].onid = ts->original_network_id;
                tp_array[count].tsid = ts->transport_stream_id;

                get_flag = false;
                for (m = 0; m < count; m++) {
                    if (tp_array[m].onid == tp_array[count].onid && tp_array[m].tsid == tp_array[count].tsid) {
                        get_flag = true;
                        break;
                    }
                }
                if (!get_flag) {
                    ++count;
                } else {
                    EUTEL_ERR("repeat ts %d=%d\n", m, count);
                }
            }
        }
    }
    tp_num = count;

    for (i = 0; i < tp_num; i++) {
        index = _get_tp_index(tp_array[i].onid, tp_array[i].tsid);
        if (index >= 0) {
            pm_tp_flag[index] = 1;
            tp_array[i].tpid = ctrl->tp_array[index].tpid;
            if (GxBus_PmTpGetById(tp_array[i].tpid, &tp) == GXCORE_SUCCESS) {
                if (tp.frequency != ts_array[i].frequency
                        || tp.tp_s.symbol_rate != ts_array[i].symbol_rate
                        || (int)tp.tp_s.polar != (int)ts_array[i].polar) {
                    EUTEL_SINFO("Process: \033[32mModify TP(%d) %d/%c/%d to %d/%c/%d\033[0m\n",
                            tp.id, tp.frequency, (tp.tp_s.polar == GXBUS_PM_TP_POLAR_H) ? 'H' : 'V', tp.tp_s.symbol_rate,
                            ts_array[i].frequency, (ts_array[i].polar == RICHEPG_TS_POLAR_H) ? 'H' : 'V', ts_array[i].symbol_rate);
                    tp.frequency = ts_array[i].frequency;
                    tp.tp_s.symbol_rate = ts_array[i].symbol_rate;
                    tp.tp_s.polar = ts_array[i].polar;
                    GxBus_PmTpModify(&tp);
                }
            }
        }
    }

    for (i = 0; i < ctrl->tp_num; i++) {
        if (pm_tp_flag[i] == 0) {
            tp_id = ctrl->tp_array[i].tpid;
            ctrl->tp_array[i].tpid = swap_tp_id;
            for (j = 0; j < ctrl->tp_array[i].ch_num; j++) {
                prog_id = ctrl->tp_array[i].ch_array[j].progid;
                if (GxBus_PmProgGetById(prog_id, &prog) == GXCORE_SUCCESS) {
                    EUTEL_SINFO("Process: \033[32mChange Prog(%d) TP %d to %d\033[0m\n", prog.id, prog.tp_id, swap_tp_id);
                    prog.tp_id = swap_tp_id;
                    GxBus_PmProgInfoModify(&prog);
                }
            }
            GxBus_PmTpDelete(&tp_id, 1);
        }
    }

    for (i = 0; i < tp_num; i++) {
        if (tp_array[i].tpid > 0)
            continue;
        memset(&tp, 0, sizeof(GxBusPmDataTP));
        tp.sat_id = sat_id;
        tp.frequency = ts_array[i].frequency;
        tp.tp_s.symbol_rate = ts_array[i].symbol_rate;
        tp.tp_s.polar = ts_array[i].polar;
        if (GxBus_PmTpAdd(&tp) == GXCORE_ERROR) {
            EUTEL_ERR("add TP failed!\n");
            goto err;
        }
        tp_array[i].tpid = tp.id;
        EUTEL_SINFO("Process: \033[32mAdd new TP(%d) %d/%c/%d\033[0m\n", tp.id,
                tp.frequency, (tp.tp_s.polar == GXBUS_PM_TP_POLAR_H) ? 'H' : 'V', tp.tp_s.symbol_rate);
    }

    for (i = 0; i < tp_num; i++) {
        for (j = 0; j < ecl->channels_num; j++) {
            ecl_ch = &ecl->channels[j];
            tech_index = _get_dvb_tech(ecl_ch);
            if (tech_index < 0)
                continue;
            tech_ch = &ecl_ch->techs[tech_index];
            if (tech_ch->trans.dvb.onid == tp_array[i].onid
                    && tech_ch->trans.dvb.tsid == tp_array[i].tsid) {
                ++tp_array[i].ch_num;
            }
        }
    }

    for (i = 0; i < tp_num; i++) {
        if (tp_array[i].ch_num > 0) {
            if ((tp_array[i].ch_array = GxCore_Calloc(tp_array[i].ch_num, sizeof(RichepgProgListCH))) == NULL) {
                EUTEL_ERR("malloc failed!\n");
                goto err;
            }
        }
        tp_array[i].ch_num = 0;
    }

    for (i = 0; i < tp_num; i++) {
        for (j = 0; j < ecl->channels_num; j++) {
            ecl_ch = &ecl->channels[j];
            tech_index = _get_dvb_tech(ecl_ch);
            if (tech_index < 0)
                continue;
            tech_ch = &ecl_ch->techs[tech_index];
            if (tech_ch->trans.dvb.onid == tp_array[i].onid
                    && tech_ch->trans.dvb.tsid == tp_array[i].tsid) {
                tp_array[i].ch_array[tp_array[i].ch_num].sid = tech_ch->trans.dvb.sid;
                tp_array[i].ch_array[tp_array[i].ch_num].chid = ecl_ch->id;
                for (m = 0; m < ctrl->tp_num; m++) {
                    for (n = 0; n < ctrl->tp_array[m].ch_num; n++) {
                        if (ctrl->tp_array[m].ch_array[n].chid == ecl_ch->id) {
                            prog_id = ctrl->tp_array[m].ch_array[n].progid;
                            if (GxBus_PmProgGetById(prog_id, &prog) == GXCORE_SUCCESS) {
                                memcpy(&prog_bak, &prog, sizeof(GxBusPmDataProg));
                                tp_array[i].ch_array[tp_array[i].ch_num].progid = prog_id;
                                scramble_flag = (ecl_ch->scrambled) ? GXBUS_PM_PROG_BOOL_ENABLE : GXBUS_PM_PROG_BOOL_DISABLE;
                                name_index = richepg_get_locale_index(ecl_ch->locales_num, ecl_ch->locales);
                                if (name_index >= 0 && ecl_ch->locales[name_index].name != NULL) {
                                    prog_name = ecl_ch->locales[name_index].name;
                                } else {
                                    prog_name = "No name";
                                }

                                if ((prog.tp_id == swap_tp_id || prog.tp_id != tp_array[i].tpid)
                                        || prog.service_id != tech_ch->trans.dvb.sid) {
                                    memset(&prog, 0, sizeof(GxBusPmDataProg));
                                }

                                if (prog.id == 0) {
                                    prog.id = prog_id;
                                    prog.sat_id = sat_id;
                                    prog.tp_id = tp_array[i].tpid;
                                    prog.service_id = tech_ch->trans.dvb.sid;
                                    prog.original_id = tech_ch->trans.dvb.onid;
                                    prog.ts_id = tech_ch->trans.dvb.tsid;
                                    prog.scramble_flag = scramble_flag;
                                    strncpy((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1);
                                    _pm_chid_set(&prog, ecl_ch->id);
                                    _set_prog_params(&prog, &prog_bak);
                                    EUTEL_SINFO("Process: \033[32mProg(progid=%d:chid=%d %s) delivery is changed\033[0m\n", prog_id, ecl_ch->id, prog_name);
                                    GxBus_PmProgInfoModify(&prog);

                                    if (app_eutel_maint_collect_check()) {
                                        struct eutelsat_lineup_channel *elld_ch = NULL;
                                        int prog_index = 0;

                                        prog_index = _get_elld_index(elld, ecl_ch->id);
                                        if (prog_index >= 0)
                                            elld_ch = &elld->channels[prog_index];
                                        app_eutel_collect_prog_info(app_eutel_collect_add_item_ch_mod, prog_name, elld_ch, ecf);
                                    }
                                } else if (prog.scramble_flag != scramble_flag
                                        || strncmp((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1) != 0) {
                                    prog.scramble_flag = scramble_flag;
                                    strncpy((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1);
                                    GxBus_PmProgInfoModify(&prog);
                                }
                            }
                            break;
                        }
                    }
                }
                ++tp_array[i].ch_num;
            }
        }
    }

    tmp_num = ctrl->tp_num;
    tmp_array = ctrl->tp_array;
    ctrl->tp_num = tp_num;
    ctrl->tp_array = tp_array;
    tp_num = tmp_num;
    tp_array = tmp_array;

    ret = 0;
err:
    if (swap_tp_id) {
        GxBus_PmTpDelete(&swap_tp_id, 1);
    }
    if (pm_tp_flag)
        GxCore_Free(pm_tp_flag);
    if (ts_array)
        GxCore_Free(ts_array);
    if (tp_array) {
        for (i = 0; i < tp_num; i++) {
            if (tp_array[i].ch_array)
                GxCore_Free(tp_array[i].ch_array);
        }
        GxCore_Free(tp_array);
    }
    if (ret < 0)
        app_eutel_proglist_tp_release();
    GxBus_PmSync(GXBUS_PM_SYNC_PROG);
    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return ret;
}

#else
static int app_eutel_proglist_tp_process_nit(struct richepg_homets *homets, struct eutelsat_channel_list *ecl,
        struct eutelsat_lineup_data *elld, struct eutelsat_configuration_file *ecf)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    struct eutelsat_channel *ecl_ch = NULL;
    struct eutelsat_techchannel *tech_ch = NULL;
    struct richepg_nit_ts *ts = NULL;
    GxBusPmDataTP tp = {0};
    uint32_t prog_id = 0;
    uint32_t tp_id = 0;
    int sat_id = 0;
    int count = 0;
    int index = 0;
    int tech_index = 0;
    int i = 0, j = 0, k = 0;
    bool get_flag = false;
    GxBusPmDataProg prog = {0};
    GxBusPmDataProgSwitch scramble_flag;
    int name_index = 0;
    char *prog_name = NULL;
    int ret = -1;

    int tp_num = 0;
    RichepgProgListTP *tp_array = NULL;
    RichepgTsTP *ts_array = NULL;
    char *pm_tp_flag = NULL; // 0
    int tmp_num = 0;
    RichepgProgListTP *tmp_array = NULL;

    app_eutel_check_ecl_delivery(ecl);

    if (app_eutel_proglist_tp_process_pm(ecl) < 0) {
        EUTEL_ERR("app_eutel_proglist_tp_process_pm failed!\n");
        return -1;
    }
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            count += homets->nit[i].ts_count;
        }
    }
    tp_num = count;
    if (tp_num == 0) {
        EUTEL_ERR("no tp!\n");
        goto err;
    }
    if ((tp_array = GxCore_Calloc(tp_num, sizeof(RichepgProgListTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    if ((ts_array = GxCore_Calloc(tp_num, sizeof(RichepgTsTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    if (ctrl->tp_num > 0 && (pm_tp_flag = GxCore_Calloc(ctrl->tp_num, sizeof(char))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }

    count = 0;
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            for (j = 0; j < homets->nit[i].ts_count; j++) {
                ts = &homets->nit[i].ts[j];
                ts_array[count].frequency = ts->frequency;
                ts_array[count].symbol_rate = ts->symbol_rate;
                if(0 == ts->polar || 2 == ts->polar)
                    ts_array[count].polar = RICHEPG_TS_POLAR_H;
                else if(1 == ts->polar || 3 == ts->polar)
                    ts_array[count].polar = RICHEPG_TS_POLAR_V;
                else
                    ts_array[count].polar = RICHEPG_TS_POLAR_H;
                tp_array[count].onid = ts->original_network_id;
                tp_array[count].tsid = ts->transport_stream_id;

                get_flag = false;
                for (k = 0; k < count; k++) {
                    if (tp_array[k].onid == tp_array[count].onid && tp_array[k].tsid == tp_array[count].tsid) {
                        get_flag = true;
                        break;
                    }
                }
                if (!get_flag) {
                    ++count;
                } else {
                    EUTEL_ERR("repeat ts %d=%d\n", k, count);
                }
            }
        }
    }
    tp_num = count;

    for (i = 0; i < tp_num; i++) {
        index = _get_tp_index(tp_array[i].onid, tp_array[i].tsid);
        if (index >= 0) {
            pm_tp_flag[index] = 1;
            tp_array[i].tpid = ctrl->tp_array[index].tpid;
            tp_array[i].ch_num = ctrl->tp_array[index].ch_num;
            tp_array[i].ch_array = ctrl->tp_array[index].ch_array;
            ctrl->tp_array[index].ch_num = 0;
            ctrl->tp_array[index].ch_array = NULL;
            if (GxBus_PmTpGetById(tp_array[i].tpid, &tp) == GXCORE_SUCCESS) {
                if (tp.frequency != ts_array[i].frequency
                        || tp.tp_s.symbol_rate != ts_array[i].symbol_rate
                        || (int)tp.tp_s.polar != (int)ts_array[i].polar) {
                    tp.frequency = ts_array[i].frequency;
                    tp.tp_s.symbol_rate = ts_array[i].symbol_rate;
                    tp.tp_s.polar = ts_array[i].polar;
                    GxBus_PmTpModify(&tp);
                }
            }
        }
    }

    for (i = 0; i < ctrl->tp_num; i++) {
        if (pm_tp_flag[i] == 0) {
            tp_id = ctrl->tp_array[i].tpid;
            GxBus_PmTpDelete(&tp_id, 1);
            if (ctrl->tp_array[i].ch_array)
                GxCore_Free(ctrl->tp_array[i].ch_array);
            memset(&ctrl->tp_array[i], 0, sizeof(RichepgProgListTP));
        }
    }
    GxBus_PmSync(GXBUS_PM_SYNC_TP);

    tmp_num = ctrl->tp_num;
    tmp_array = ctrl->tp_array;
    ctrl->tp_num = tp_num;
    ctrl->tp_array = tp_array;
    tp_num = tmp_num;
    tp_array = tmp_array;

    sat_id = app_eutel_cur_sat_id_get();
    for (i = 0; i < ctrl->tp_num; i++) {
        if (ctrl->tp_array[i].tpid > 0)
            continue;
        memset(&tp, 0, sizeof(GxBusPmDataTP));
        tp.sat_id = sat_id;
        tp.frequency = ts_array[i].frequency;
        tp.tp_s.symbol_rate = ts_array[i].symbol_rate;
        tp.tp_s.polar = ts_array[i].polar;
        if (GxBus_PmTpAdd(&tp) == GXCORE_ERROR) {
            EUTEL_ERR("add TP failed!\n");
            goto err;
        }
        ctrl->tp_array[i].tpid = tp.id;
    }
    GxBus_PmSync(GXBUS_PM_SYNC_TP);

    for (i = 0; i < ctrl->tp_num; i++) {
        for (j = 0; j < ctrl->tp_array[i].ch_num; j++) {
            prog_id = ctrl->tp_array[i].ch_array[j].progid;
            get_flag = false;
            for (k = 0; k < ecl->channels_num; k++) {
                ecl_ch = &ecl->channels[k];
                tech_index = _get_dvb_tech(ecl_ch);
                if (tech_index < 0)
                    continue;
                tech_ch = &ecl_ch->techs[tech_index];
                if (tech_ch->trans.dvb.sid == ctrl->tp_array[i].ch_array[j].sid
                        && tech_ch->trans.dvb.onid == ctrl->tp_array[i].onid
                        && tech_ch->trans.dvb.tsid == ctrl->tp_array[i].tsid) {
                    if (GxBus_PmProgGetById(prog_id, &prog) == GXCORE_SUCCESS) {
                        scramble_flag = (ecl_ch->scrambled) ? GXBUS_PM_PROG_BOOL_ENABLE : GXBUS_PM_PROG_BOOL_DISABLE;
                        name_index = richepg_get_locale_index(ecl_ch->locales_num, ecl_ch->locales);
                        if (name_index >= 0 && ecl_ch->locales[name_index].name != NULL) {
                            prog_name = ecl_ch->locales[name_index].name;
                        } else {
                            prog_name = "No name";
                        }

                        if (prog.scramble_flag != scramble_flag
                                || strncmp((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1) != 0) {
                            prog.scramble_flag = scramble_flag;
                            strncpy((char*)prog.prog_name, prog_name, MAX_PROG_NAME-1);
                            GxBus_PmProgInfoModify(&prog);
                        }
                    }
                    get_flag = true;
                    break;
                }
            }
            if (!get_flag) {
                GxBus_PmProgDelete(&prog_id, 1);
                memset(&ctrl->tp_array[i].ch_array[j], 0, sizeof(RichepgProgListCH));
            }
        }
    }
    GxBus_PmSync(GXBUS_PM_SYNC_PROG);

    ret = 0;
err:
    if (pm_tp_flag)
        GxCore_Free(pm_tp_flag);
    if (ts_array)
        GxCore_Free(ts_array);
    if (tp_array)
        GxCore_Free(tp_array);
    if (ret < 0)
        app_eutel_proglist_tp_release();
    return ret;
}
#endif

#else
static int app_eutel_proglist_tp_process_nit(struct richepg_homets *homets, struct eutelsat_channel_list *ecl,
        struct eutelsat_lineup_data *elld, struct eutelsat_configuration_file *ecf)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    struct richepg_nit_ts *ts = NULL;
    GxBusPmDataTP tp = {0};
    int sat_id = 0;
    int count = 0;
    int i = 0, j = 0;

    if (app_eutel_proglist_tp_delete() < 0)
        return -1;
    app_eutel_proglist_tp_release();
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            count += homets->nit[i].ts_count;
        }
    }
    if (count == 0)
        return -1;
    if ((ctrl->tp_array = GxCore_Calloc(count, sizeof(RichepgProgListTP))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        return -1;
    }

    count = 0;
    sat_id = app_eutel_cur_sat_id_get();
    for (i = 0; i < homets->nit_count; i++) {
        if (homets->nit[i].got) {
            for (j = 0; j < homets->nit[i].ts_count; j++) {
                ts = &homets->nit[i].ts[j];
                memset(&tp, 0, sizeof(GxBusPmDataTP));
                tp.sat_id = sat_id;
                tp.frequency = ts->frequency;
                tp.tp_s.symbol_rate = ts->symbol_rate;
                if(0 == ts->polar || 2 == ts->polar)
                    tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
                else if(1 == ts->polar || 3 == ts->polar)
                    tp.tp_s.polar = GXBUS_PM_TP_POLAR_V;
                else
                    tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
                if (GxBus_PmTpAdd(&tp) == GXCORE_ERROR) {
                    EUTEL_ERR("add TP failed!\n");
                    goto err;
                }

                ctrl->tp_array[count].onid = ts->original_network_id;
                ctrl->tp_array[count].tsid = ts->transport_stream_id;
                ctrl->tp_array[count].tpid = tp.id;
                ++count;
            }
        }
    }
    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    ctrl->tp_num = count;

    return 0;
err:
    app_eutel_proglist_tp_release();
    return -1;
}
#endif

int app_eutel_proglist_update(bool store_ch, bool del_all)
{
    struct richepg_homets *homets = NULL;
    struct eutelsat_channel_list *ecl  = NULL;
    struct eutelsat_lineup_data *elld = NULL;
    struct eutelsat_configuration_file *ecf = NULL;
    struct eutelsat_additional_resources *ear = NULL;

    if (richepg_get_homets(&homets) < 0)
        goto err0;
    if (homets->nit_count == 0)
        goto err0;
    if (richepg_get_ecl(&ecl) < 0)
        goto err0;
    if (richepg_get_elld(&elld) < 0)
        goto err0;
    if (richepg_get_ecf(&ecf) < 0)
        goto err0;
    richepg_get_ear(&ear);
    if (elld->channel_num == 0)
        goto err0;
    if (del_all && app_eutel_proglist_tp_delete() < 0)
        goto err0;
    if (app_eutel_proglist_tp_process_nit(homets, ecl, elld, ecf) < 0)
        goto err;
    app_eutel_collect_channel_genres(ecl, elld, ecf);
    if (app_eutel_proglist_prog_process(ecl, elld, ecf, ear, 1) < 0)
        goto err;
    if (store_ch) {
        if (richepg_proglist_save() < 0)
            goto err;
    } else {
        richepg_proglist_release();
    }

    app_eutel_proglist_tp_release();
    richepg_free_homets();
    return 0;

err:
    app_eutel_proglist_tp_release();
    richepg_proglist_release();
err0:
    richepg_free_homets();
    return -1;
}

int app_eutel_proglist_update2(void)
{
    struct eutelsat_channel_list *ecl  = NULL;
    struct eutelsat_lineup_data *elld = NULL;
    struct eutelsat_additional_resources *ear = NULL;
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();

    if (richepg_get_ecl(&ecl) < 0)
        goto err0;
    if (richepg_get_elld(&elld) < 0)
        goto err0;
    richepg_get_ear(&ear);
    if (elld->channel_num == 0)
        goto err0;
    if (app_eutel_proglist_tp_process_pm(NULL) < 0 || ctrl->tp_num == 0)
        goto err0;
    if (app_eutel_proglist_prog_process(ecl, elld, NULL, ear, 0) < 0)
        goto err;
    if (richepg_proglist_save() < 0)
        goto err;

    app_eutel_proglist_tp_release();
    return 0;

err:
    app_eutel_proglist_tp_release();
    richepg_proglist_release();
err0:
    return -1;
}

int app_eutel_proglist_ear_update(void)
{
    RichepgProgListCtrl *ctrl = richepg_prog_list_ctrl_get();
    int i = 0;
    char *ear_path = NULL;

    if (ctrl->prog_num == 0) {
        if (richepg_proglist_parse() < 0)
            return -1;
        if (ctrl->prog_num == 0)
            return -1;
    }

    block_mem_mgr_free_data(&ctrl->ear_mgr);
    for (i = 0; i < ctrl->prog_num; i++) {
        ear_path = richepg_get_ear_channel_logo(ctrl->prog_array[i].id);
        if (ear_path) {
            ctrl->prog_array[i].ear_logo = block_mem_mgr_dup_str(ear_path, &ctrl->ear_mgr);
        } else {
            ctrl->prog_array[i].ear_logo = NULL;
        }
    }
    block_mem_mgr_adjust_size(&ctrl->ear_mgr);

    if (richepg_proglist_save() < 0)
        goto err;

    return 0;
err:
    return -1;
}

int app_eutel_database_reset(void)
{
    app_eutel_maint_check_stop();
    richepg_clean_all_flash_files();
    richepg_ts_mgr_delfile();
    richepg_proglist_delfile();
    app_richepg_config_reset();
    app_eutel_collect_reset();
    return 0;
}
#endif
