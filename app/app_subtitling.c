/*
 * =====================================================================================
 *
 *       Filename:  app_subtitling.c
 *
 *    Description:  select dvb subtitle or ttx subtitle
 *
 *        Version:  1.0
 *        Created:  2011年10月13日 10时14分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_send_msg.h"
#include "app_ui_style.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_common_select.h"
#include "app_channel_info.h"
#include "channel_common.h"

#define SUBT_OFF_MODE_NUM    1

#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"
static int subttran_switch_sel = 0;
#endif

typedef struct subtitling Subtitling;
struct subtitling
{
    int32_t     sel;
    uint32_t    mode;
    char(*lang)[8];
    char(*lang_list)[8];
    void (*magz_open)(void);
    status_t (*open)(Subtitling *, uint32_t);
    void (*close)(Subtitling *);
    void (*change)(Subtitling *);
    void (*pause)(Subtitling *);
    void (*resume)(Subtitling *);

#define SUBT_MODE_OFF   (0)
#define SUBT_MODE_TTX   (1)
#define SUBT_MODE_DVB   (2)
#define SUBT_MODE_PAUSE (3)
#define SUBT_MODE_ARIBCC (4)

};

extern Subtitling g_AppSubtitling;
static event_list *timer_refresh_list = NULL;
static char s_ch_setfocus = 0;//0:Don't do set sel for g_CommonSelect.widget.list 1:do
#if ARIBCC_SUPPORT
extern char *app_aribcc_get_caption_lang(int index);
extern void app_aribcc_sub_display(bool bDisplay );
#endif
#if ATSCCC_SUPPORT
extern bool app_atsc_mode_get(void);
extern void app_atsc_close_sub(void);
extern void app_atsc_switchservice(int sel);
extern int app_atsc_get_num(void);
#endif
extern char *app_get_short_lang_str(uint32_t lang);

static int _check_char_validity(uint8_t ch_str)//language code from ISO-8859-1
{
    uint8_t tmp = 0;

    tmp = (ch_str & 0xf0) >> 4;
    if ((tmp == 0) || (tmp == 1) || (tmp == 8) || (tmp == 9))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int _refresh_list(void *usrdata)
{
    uint32_t            i = 0;
    TtxSubtOps          *p = &g_AppTtxSubt;
    Subtitling          *p_sub = &g_AppSubtitling;
    static uint32_t     last_lang = 0;
    uint32_t            lang = 0;
    int32_t             sel = 0;
    uint32_t            total_sub = 0;
    total_sub = p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM;
#if ARIBCC_SUPPORT
    total_sub += p->aribcc_num;
#endif
    uint32_t size = sizeof(char *[8]) * (total_sub);
    p_sub->lang = GxCore_Malloc(size);
    if (p_sub->lang == NULL)
    {
        return EVENT_TRANSFER_STOP;
    }

    memset(p_sub->lang, 0, size);
    lang++;

    if(total_sub != 0)
    {
        strncpy((char *)p_sub->lang, STR_ID_OFF, 8);
    }
    else
    {
        sel = 0;
        strncpy((char *)p_sub->lang, STR_ID_NO_DATA, 8);
    }

    for (i = 0; i < p->ttx_num; i++)
    {
        if (_check_char_validity(p->content[lang - 1].lang[0]))
        {
            p->content[lang - 1].lang[0] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[1]))
        {
            p->content[lang - 1].lang[1] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[2]))
        {
            p->content[lang - 1].lang[2] = 0x20;
        }

        sprintf((char *)(p_sub->lang + lang), "%c%c%c(T)", p->content[lang - 1].lang[0],
                p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);
        lang++;
    }
    for (i = 0; i < p->subt_num; i++)
    {
        if (_check_char_validity(p->content[lang - 1].lang[0]))
        {
            p->content[lang - 1].lang[0] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[1]))
        {
            p->content[lang - 1].lang[1] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[2]))
        {
            p->content[lang - 1].lang[2] = 0x20;
        }
        if((p->content[lang - 1].type >= 0x20)&&(p->content[lang - 1].type <= 0x24))
        {
            sprintf((char *)(p_sub->lang + lang), "%c%c%c(H)", p->content[lang - 1].lang[0],
                    p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);
        }
        else
        {
            sprintf((char *)(p_sub->lang + lang), "%c%c%c(D)", p->content[lang - 1].lang[0],
                p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);

        }
        lang++;
    }
    #if ARIBCC_SUPPORT
    for (i = 0; i < p->aribcc_num; i++)
    {
        if (_check_char_validity(p->content[lang - 1].lang[0]))
        {
            p->content[lang - 1].lang[0] = 0x50;//0x41;
        }
        if (_check_char_validity(p->content[lang - 1].lang[1]))
        {
            p->content[lang - 1].lang[1] = 0x6f;//0x72;
        }
        if (_check_char_validity(p->content[lang - 1].lang[2]))
        {
            p->content[lang - 1].lang[2] = 0x6c;//0x69;
        }

        sprintf((char *)(p_sub->lang + lang), "%c%c%c(CC)", p->content[lang - 1].lang[0],
                p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);
        lang++;
    }
    #endif

    if (s_ch_setfocus == 1 || last_lang != lang)
    {
        if (lang != 1)
        {
            sel = p_sub->sel;
            if (lang <= sel)
            {
                sel = 1;
            }
            if(g_AppSubtitling.mode == SUBT_MODE_OFF)
            {
                sel = 0;
            }
        }

        GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
        GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
    }
    app_update_pop_dlg(WND_PASSWD);

    s_ch_setfocus = 0;
    last_lang = lang;

    if ((timer_refresh_list != NULL) && (p->sync_flag == 1))
    {
        remove_timer(timer_refresh_list);
        timer_refresh_list = NULL;
    }

    return GXCORE_SUCCESS;
}

uint32_t app_get_subtitling_mode(void)
{
    return g_AppSubtitling.mode;
}

void app_ttx_magz_open(void)
{
    PopDlg pop = {0};
    GxMsgProperty_NodeByPosGet node;
    TtxSubtOps *p_ttx = &g_AppTtxSubt;

    pop.type = POP_TYPE_OK;
    pop.str = STR_ID_NO_TELTEXT;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;

    if (g_AppChOps.get_list_total() > 0)
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        if (node.prog_data.ttx_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        {
            if (g_AppPvrOps.state != PVR_DUMMY)
            {
                app_pvr_create_no_btn_popdlg(NULL);
            }
            else
            {
                if(p_ttx->magazine.elem_pid != 0)
                {
                    if(g_AppFullArb.state.mute == STATE_ON)
                    {
                        GUI_SetProperty("img_full_mute", "state", "hide");
                    }
                    app_subt_pause();
                    GUI_EndDialog(WND_VOLUME);
                    app_clean_fullscreen_tip(false, false, true);
                    g_AppFullArb.timer_stop();
                    GUI_SetInterface("flush", NULL);//及时提交UI改动
                    p_ttx->ttx_magz_open(p_ttx);
                }
                else
                {
                    popdlg_create(&pop);
                }
            }
        }
        else
        {
            if (g_AppPvrOps.state != PVR_DUMMY)
            {
                app_pvr_create_no_btn_popdlg(NULL);
            }
            else
            {
                popdlg_create(&pop);

            }
        }
    }
}

void app_subt_change(void)
{
    if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS
            || (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
            || (app_channel_epg_check_dialog() == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
            || g_AppFullArb.state.tv == STATE_OFF)
    {
        return;
    }

    g_AppSubtitling.change(&g_AppSubtitling);
}

void app_subt_close(void)
{
    g_AppSubtitling.close(&g_AppSubtitling);
}

void app_subt_pause(void)
{
    g_AppSubtitling.pause(&g_AppSubtitling);
}

void app_subt_resume(void)
{
    TtxSubtOps *p = &g_AppTtxSubt;
     if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS
        || (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
        || (app_channel_epg_check_dialog() == GXCORE_SUCCESS)
        || (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        || (g_AppFullArb.state.tv == STATE_OFF))
    {
        return;
    }
#if ARIBCC_SUPPORT
     if (p->ttx_num + p->subt_num + p->aribcc_num> 0)
#else
    if (p->ttx_num + p->subt_num > 0)
#endif
    {
        g_AppSubtitling.resume(&g_AppSubtitling);
    }
}
#ifndef COLOR_16BIT_TTX_SUPPORT
void app_old_ttx_pause(void)
{
     TtxSubtOps *p = &g_AppTtxSubt;
    if(g_AppSubtitling.mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }
}
#endif

static void app_subtitling_save_extinfo(uint32_t sel)
{
    TtxSubtOps *p = &g_AppTtxSubt;
    AppProgExtInfo prog_ext_info;
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;

    if (g_AppFullArb.state.pause == STATE_ON)
    {
        return;
    }
    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
#if ARIBCC_SUPPORT
    if (sel > p->ttx_num + p->subt_num + p->aribcc_num || sel == 0)
#else
    if (sel > p->ttx_num + p->subt_num || sel == 0)
#endif
    {
        return;
    }

    if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        #if LCN_SUPPORT
        GxBus_PmProgExtInfoGet(node.prog_data.id, sizeof ( AppProgExtInfo ), &prog_ext_info );
        #endif
        memcpy(&prog_ext_info.subt, &p->content[sel - SUBT_OFF_MODE_NUM], sizeof(ttx_subt));
        GxBus_PmProgExtInfoAdd(node.prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
        GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
    }

    return;
}

static status_t app_subtitling_open(Subtitling *p_sub, uint32_t sel)
{
    status_t ret = GXCORE_ERROR;
    TtxSubtOps *p = &g_AppTtxSubt;
    uint32_t total_pmtsub = 0;

    if (g_AppFullArb.state.pause == STATE_ON)
    {
        return GXCORE_ERROR;
    }
    total_pmtsub = p->ttx_num + p->subt_num;
#if ARIBCC_SUPPORT
    total_pmtsub += p->aribcc_num;
#endif
    if (sel > total_pmtsub || sel == 0)
    {
        // sel = 0 mean MODE_OFF
        return GXCORE_ERROR;
    }

    //exit the pre-process for release the TTX memory for subt
    g_AppTtxSubt.ttx_pre_process_close(&g_AppTtxSubt);
    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }
#if ARIBCC_SUPPORT
    if (p_sub->mode == SUBT_MODE_ARIBCC)
    {
        p->aribcc_subt_close(p);
    }
#endif

    if (sel > p->ttx_num)
    {

#if ARIBCC_SUPPORT
        if(sel > p->subt_num+p->ttx_num)
        {
            ret = p->aribcc_subt_open(p, sel - SUBT_OFF_MODE_NUM - p->ttx_num);
            if (ret == GXCORE_SUCCESS)
            {
                p_sub->mode = SUBT_MODE_ARIBCC;
            }
        }
        else
#endif
        {
            // dvb_subt
            ret = p->dvb_subt_open(p, sel - SUBT_OFF_MODE_NUM - p->ttx_num);
            if (ret == GXCORE_SUCCESS)
            {
                p_sub->mode = SUBT_MODE_DVB;
            }
        }
    }
    else
    {
        // ttx_subt
        ret = p->ttx_subt_open(p, sel - SUBT_OFF_MODE_NUM);
        if (ret == GXCORE_SUCCESS)
        {
            p_sub->mode = SUBT_MODE_TTX;
        }
    }

    if (ret == GXCORE_SUCCESS)
    {
        g_AppFullArb.state.subt = STATE_ON;
        p_sub->sel = sel;
#ifdef SUBTTRANS_SUPPORT
        if((p_sub->mode == SUBT_MODE_DVB) && (1 == Gx_SubtTransGetStatus()))
        {
            uint16_t service_id = 0;
            GxMsgProperty_NodeByPosGet node = {0};
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node))){
                service_id = node.prog_data.service_id;
            }
            p->dvb_subt_close(p);
            Gx_SubtTrans_SetSubtPid(p->content[p_sub->sel-1].elem_pid, (char *)p->content[p_sub->sel-1].lang, service_id, p_sub->mode);
            Gx_SubtTrans_ChannelSwitch();
            Gx_SubtTrans_DrawClean();
        }
        else
        {
            if((1 == Gx_SubtTransGetStatus())){
                PopDlg pop = {0};
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.str = STR_ID_ST_AUTO_OFF;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.timeout_sec = 3;
                popdlg_create(&pop);
                Gx_SubtTransStop();
                Gx_SubtTrans_Replay_ByTsbuff();
            }
        }
#endif
    }
    else
    {
        //APP_DBG(APP_SUBT, "open subt[%d] failed !!", sel);
        g_AppFullArb.state.subt = STATE_OFF;

        p_sub->sel = 0;
        p_sub->mode = SUBT_MODE_OFF;
    }

    return ret;
}

static void app_subtitling_close(Subtitling *p_sub)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }
#if ARIBCC_SUPPORT
    if (p_sub->mode == SUBT_MODE_ARIBCC)
    {
        p->aribcc_subt_close(p);
        app_aribcc_sub_display(FALSE);
    }
#endif

    p_sub->sel = 0;
    p_sub->mode = SUBT_MODE_OFF;

    g_AppFullArb.state.subt = STATE_OFF;

#ifdef SUBTTRANS_SUPPORT
    if(1 == Gx_SubtTransGetStatus()) {
        Gx_SubtTransStop();
        Gx_SubtTrans_Replay_ByTsbuff();
    }
#endif
    //restart the pre-process, if APP support the function.
    g_AppTtxSubt.ttx_pre_process_init(&g_AppTtxSubt);
}

#ifdef SUBTTRANS_SUPPORT
static void _subttrans_replay(void)
{
    int time = Gx_SubtTrans_GetDelayms();
    int ret = Gx_SubtTrans_Replay_ByTsbuff();
    if(ret == GXCORE_SUCCESS)
    {
        PopDlg pop = {0};
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.str = STR_ID_ST_WAIT_START;
        pop.timeout_sec = time/1000;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
}
#endif

static void app_subtitling_change(Subtitling *p_sub)
{
    char *str=NULL;
    int32_t lang = -1;
    int cur_subt_lang = 0;
    uint32_t i = 0;
    TtxSubtOps *p = &g_AppTtxSubt;
    uint32_t total_pmtsub = 0;
    int hoh_value = 0;
    p_sub->sel = 0;
    p_sub->mode = SUBT_MODE_OFF;

    total_pmtsub = p->ttx_num + p->subt_num;
#if ARIBCC_SUPPORT
    total_pmtsub += p->aribcc_num;
#endif

    if (total_pmtsub == 0)
    {
        return;
    }
    GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &cur_subt_lang, OSD_LANG);
    if(cur_subt_lang == 0)
    {
         g_AppSubtitling.close(&g_AppSubtitling);
         return;
    }
    str = app_get_short_lang_str(cur_subt_lang-1);
    for(i = 0; i < total_pmtsub; i++)
    {
        if(app_language_code_cmp(str,(char*)p->content[i].lang)==0)
        {
            GxBus_ConfigGetInt(SUBTITLE_HOH_KEY, &hoh_value, SUBTITLE_HOH_VALUE);
            if(hoh_value)
            {
                if((p->content[i].type >= 0x20) && (p->content[i].type <= 0x24))
                {
                    lang=i;
                    break;
                }
            }
            else
            {
                if((p->content[i].type < 0x20) || (p->content[i].type > 0x24))
                {
                    lang=i;
                    break;
                }
            }
        }
    }
    if(lang == -1)
    {
        p_sub->sel=0;
        g_AppSubtitling.close(&g_AppSubtitling);
        return;
    }
    p_sub->open(p_sub, lang + SUBT_OFF_MODE_NUM);

#ifdef SUBTTRANS_SUPPORT
    if(1 == Gx_SubtTransGetStatus()) {
        _subttrans_replay();
        Gx_SubtTrans_DrawResume();
    }
#endif

#if ARIBCC_SUPPORT
    app_aribcc_sub_display(TRUE);
#endif
}

static void app_subtitling_pause(Subtitling *p_sub)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }
#if ARIBCC_SUPPORT
    if (p_sub->mode == SUBT_MODE_ARIBCC)
    {
        p->aribcc_subt_close(p);
    }
#endif

    if(SUBT_MODE_OFF != p_sub->mode)
    {
        g_AppFullArb.state.subt = STATE_OFF;
        p_sub->mode = SUBT_MODE_PAUSE;
#ifdef SUBTTRANS_SUPPORT
        Gx_SubtTrans_DrawPause();
#endif
    }
}

static void app_subtitling_resume(Subtitling *p_sub)
{
    int32_t lang = 0;
    TtxSubtOps *p = &g_AppTtxSubt;
    #if ARIBCC_SUPPORT
    if (p_sub->mode != SUBT_MODE_OFF
            && p->ttx_num + p->subt_num + p->aribcc_num> lang)
    #else
    if (p_sub->mode != SUBT_MODE_OFF
            && p->ttx_num + p->subt_num > lang)
    #endif
    {
        p_sub->open(p_sub, p_sub->sel);
#ifdef SUBTTRANS_SUPPORT
        Gx_SubtTrans_DrawResume();
#endif
    }
}

static int _subtitling_list_get_total(void)
{
#if ATSCCC_SUPPORT
    if(app_atsc_mode_get() == true)
    {
        uint32_t atsccc_num = 0;
        atsccc_num = app_atsc_get_num();
        return atsccc_num + SUBT_OFF_MODE_NUM;
    }
    else
#endif
    {
        TtxSubtOps *p = &g_AppTtxSubt;
        uint32_t total_pmtnum = 0;
        total_pmtnum = p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM;
#if ARIBCC_SUPPORT
        total_pmtnum += p->aribcc_num;
#endif
#ifdef SUBTTRANS_SUPPORT
        if(total_pmtnum > 1){
            total_pmtnum++;
            subttran_switch_sel = total_pmtnum-1;
        }
#endif
        return total_pmtnum;
    }
}

static int app_subtitling_list_get_total(GuiWidget *widget, void *usrdata)
{
    return _subtitling_list_get_total();
}

static int app_subtitling_list_get_data(GuiWidget *widget, void *usrdata)
{
#define SUB_NUM_LEN     (10)
    static char buffer[SUB_NUM_LEN];
    ListItemPara *item = NULL;
    item = (ListItemPara *)usrdata;
    if (NULL == item)
    {
        return GXCORE_ERROR;
    }
    if (0 > item->sel)
    {
        return GXCORE_ERROR;
    }
    // col-0: num
    item->x_offset = 0;
    item->image = NULL;
    if (item->sel > 0)
    {
        memset(buffer, 0, SUB_NUM_LEN);
        sprintf(buffer, "%d", item->sel);
        item->string = buffer;
    }
    else
    {
        item->string = NULL;
    }
#if ATSCCC_SUPPORT
    static char ccbuf[SUB_NUM_LEN];
    if(app_atsc_mode_get() == true)
    {
        memset(ccbuf, 0, SUB_NUM_LEN);
        if(item->sel == 0)
        {
            if(app_atsc_get_num() == 0)
            {
                strncpy(ccbuf, STR_ID_NO_DATA, SUB_NUM_LEN);
            }
            else
            {
                strncpy(ccbuf, STR_ID_OFF, SUB_NUM_LEN);
            }
        }
        else
        {
            sprintf(ccbuf, "CC(%d)", item->sel);
        }
        item = item->next;
        item->image = NULL;
        item->string = ccbuf;
    }
    else
#endif
    {
#ifdef SUBTTRANS_SUPPORT
        if(item->sel == subttran_switch_sel)
        {
            item = item->next;
            item->image = NULL;
            if(0 == Gx_SubtTransGetStatus())
                item->string = STR_ID_SUBTTRANS_SWITCH_OFF;
            else
                item->string = STR_ID_SUBTTRANS_SWITCH_ON;
        }
        else
#endif
        {
        Subtitling  *p_sub = &g_AppSubtitling;

#if ARIBCC_SUPPORT
        char *arib_lang =NULL;
        TtxSubtOps *p = &g_AppTtxSubt;

        arib_lang = app_aribcc_get_caption_lang(item->sel+1);
        if((arib_lang!=NULL)&&(item->sel !=0)
                &&( p->aribcc_num> 0))
        {
            memcpy(p_sub->lang[item->sel],arib_lang,3);
        }
#endif
        //col-1: channel name
        item = item->next;
        item->image = NULL;
        item->string = p_sub->lang[item->sel];
        }
    }
#ifdef WND_COMMONSELECT_CHOINCESHOW_SUPORT
    uint32_t active_sel = 0;
    //col-.3 select status
    GUI_GetProperty(g_CommonSelect.widget.list, "active", &active_sel);
    item = item->next;
    if((item->sel == active_sel)&&(strcmp(g_AppSubtitling.lang[item->sel],STR_ID_NO_DATA) != 0))
    {
        item->image = "s_choice";
    }
    else
    {
        item->image = NULL;
    }
    item->string = NULL;
#endif

    return EVENT_TRANSFER_STOP;
}

static int app_subtitling_list_create(GuiWidget *widget, void *usrdata)
{
#if ATSCCC_SUPPORT
    if(app_atsc_mode_get() != true)
#endif
    {
        TtxSubtOps          *p = &g_AppTtxSubt;
#ifdef SUBTTRANS_SUPPORT
        subttran_switch_sel = -1;
#endif
        s_ch_setfocus = 1;
        _refresh_list(NULL);

        if (p->sync_flag == 0)
        {
            timer_refresh_list = create_timer(_refresh_list, 2000, 0, TIMER_REPEAT);
        }
    }
    return EVENT_TRANSFER_STOP;
}

static int app_subtitling_list_destroy(GuiWidget *widget, void *usrdata)
{
#if ATSCCC_SUPPORT
    if(app_atsc_mode_get() != true)
#endif
    {
        Subtitling  *p_sub = &g_AppSubtitling;
        GxCore_Free(p_sub->lang);

        if (timer_refresh_list)
        {
            remove_timer(timer_refresh_list);
            timer_refresh_list = NULL;
        }
    }

    return EVENT_TRANSFER_STOP;
}

#ifdef SUBTTRANS_SUPPORT
static void _subt_subttrans_dvbsubt_close(void)
{
    Subtitling  *p_sub = &g_AppSubtitling;
    TtxSubtOps *p = &g_AppTtxSubt;
    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }
    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }
    g_AppFullArb.state.subt = STATE_OFF;
}

static int _subt_subttrans_switch_cb(PopDlgRet ret)
{
    GxMsgProperty_NodeByPosGet node = {0};
    Subtitling  *p_sub = &g_AppSubtitling;
    TtxSubtOps *p_ttx = &g_AppTtxSubt;
    uint16_t service_id = 0;

    if((ret == POP_VAL_OK) && (p_sub->mode != SUBT_MODE_OFF))
    {
        if(0 == Gx_SubtTransGetStatus())
        {
            _subt_subttrans_dvbsubt_close();
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node))){
                service_id = node.prog_data.service_id;
            }
            Gx_SubtTrans_SetSubtPid(p_ttx->content[p_sub->sel-1].elem_pid, (char *)p_ttx->content[p_sub->sel-1].lang, service_id, p_sub->mode);
            Gx_SubtTransStart();
            _subttrans_replay();
        }
        else
        {
            Gx_SubtTransStop();
            _subttrans_replay();
            app_subtitling_save_extinfo(p_sub->sel);
            p_sub->open(p_sub, p_sub->sel);
        }
    }
    return 0;
}
static void app_subtitling_subttrans_switch_popdlg(int mode)
{
    Subtitling  *p_sub = &g_AppSubtitling;
    PopDlg pop = {0};
    memset(&pop, 0, sizeof(PopDlg));

    if((mode == SUBT_MODE_OFF) || (p_sub->mode != SUBT_MODE_DVB))
    {
        pop.type = POP_TYPE_OK;
        pop.timeout_sec = 2;
        if((mode == SUBT_MODE_OFF))
            pop.str = STR_ID_ST_ENABLE_SUBT_STR;
        else
            pop.str = STR_ID_SUBTTRANS_NOT_SUPPORT;
    }
    else
    {
        if(0 == Gx_SubtTransActivate_Get_Status(NULL, 0))
        {
            pop.type = POP_TYPE_YES_NO;
            pop.exit_cb = _subt_subttrans_switch_cb;
            if(0 == Gx_SubtTransGetStatus()) {
                pop.str = STR_ID_ST_ENABLE_ST_STR;
            } else {
                pop.str = STR_ID_ST_DISABLE_ST_STR;
            }
        }
        else
        {
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_ST_ACT_ST_STR;
            pop.timeout_sec = 2;
        }
    }
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    popdlg_create(&pop);
}
#endif
static int app_subtitling_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    Subtitling  *p_sub = &g_AppSubtitling;
    int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch (find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    app_channel_info_create_dialog();
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                        if (sel == 0)
                        {
                            sel = _subtitling_list_get_total() - 1;
                            GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                        if (_subtitling_list_get_total() == sel+1)
                        {
                            sel = 0;
                            GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                case STBK_OK:
                {

                    int32_t sel;
                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
#if ATSCCC_SUPPORT
                    if(app_atsc_mode_get() == true)
                    {
                        if(app_atsc_get_num() != 0)
                        {
                            if(sel == 0)
                            {
                                app_atsc_close_sub();
                            }
                            else
                            {
                                sel -= 1;
                                app_atsc_switchservice(sel);
                            }
                        }
                    }
                    else
#endif
                    {
                        TtxSubtOps *p = &g_AppTtxSubt;
                        uint32_t total = p->ttx_num + p->subt_num;
#if ARIBCC_SUPPORT
                        total +=  p->aribcc_num;
#endif

                        if (total != 0)
                        {
                            if (sel == 0)
                            {
                                p_sub->close(p_sub);
                            }
                            else
                            {
#ifdef SUBTTRANS_SUPPORT
                                if(sel == subttran_switch_sel)
                                {
                                    app_subtitling_subttrans_switch_popdlg(p_sub->mode);
                                }
                                else
#endif
                                {
                                    app_subtitling_save_extinfo(sel);
                                    p_sub->open(p_sub, sel);
#if ARIBCC_SUPPORT
                                    app_aribcc_sub_display(TRUE);
#endif
                                }
                            }
                        }
                    }
#ifdef SUBTTRANS_SUPPORT
                    if(sel != subttran_switch_sel)
#endif
                    {
                        GUI_EndDialog(WND_COMMON_SELECT);
                        app_channel_info_create_dialog();
                    }
                }
                    break;
#ifdef SUBTTRANS_SUPPORT
                case STBK_GREEN:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
#endif
                default:
                break;
            }
    }

    return ret;
}

static int app_subtitling_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_KEYDOWN:
            switch (find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    app_channel_info_create_dialog();
                    break;
#ifdef SUBTTRANS_SUPPORT
                case STBK_GREEN:
                    {
                        app_subt_pause();
                        extern void app_subttrans_setting_menu_with_play(void);
                        app_subttrans_setting_menu_with_play();
                    }
                    break;
#endif
                default:
                    break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

Subtitling g_AppSubtitling =
{
    .sel = SUBTITLE_LANG_VALUE,
    .mode = SUBTITLE_MODE_VALUE,
    .magz_open          = app_ttx_magz_open,
    .open    = app_subtitling_open,
    .close   = app_subtitling_close,
    .change  = app_subtitling_change,
    .pause   = app_subtitling_pause,
    .resume  = app_subtitling_resume,

};

int app_subtitle_create_dialog(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = STR_ID_SUBTITLING;
#ifdef SUBTTRANS_SUPPORT
    paras.tip_strs.str_green = STR_ID_ST_TRANS;
#endif
    paras.signal.create = app_subtitling_list_create;
    paras.signal.destroy = app_subtitling_list_destroy;
    paras.signal.keypress = app_subtitling_keypress;
    paras.signal.list_get_total = app_subtitling_list_get_total;
    paras.signal.list_get_data = app_subtitling_list_get_data;
    paras.signal.list_keypress = app_subtitling_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
