#include "channel_common.h"
#include "app_windows.h"
#include "app_channel_info.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern int app_check_tkgs_control_db(void);
#endif
#if WEBLET_SUPPORT
#include "app_weblet.h"
#endif

#define CMB_GROUP   "cmb_channel_list_group"
#define TXT_GROUP   "txt_group_sat_fav"
#define TXT_TP      "text_channel_list_tp"
#define LV_LIST     "listview_channel_list"
#define IMG_BOTTOM  "img_cl_bottom"
#define IMG_BODY    "img_cl_body"

#define IMG_RED     "img_cl_red"
#define TXT_RED     "text_cl_red"
#define IMG_GREEN   "img_cl_green"
#define TXT_GREEN   "text_cl_green"
#define IMG_YELLOW  "img_cl_yellow"
#define TXT_YELLOW  "text_cl_yellow"

/* widget macro -------------------------------------------------------- */


#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
static int s_cur_sat_sel = -1;
#endif

static GxMsgProperty_NodeByPosGet node_prog_tag = {0};
static int s_cur_fav_sel = -1;
static int s_prog_list_find_num = 0;
static uint32_t *s_prog_list_find_id_array = NULL;

typedef struct channel_list ChannelList;
struct channel_list
{
    uint32_t first_in;// first in, when cmb change needn't play program again
    ListMode mode;
    AppIdOps  *id_ops[LIST_MODE_TOTAL];
    void (*init)(ChannelList*, ListMode);
};

extern void channel_list_init(ChannelList *list, ListMode mode);
extern void app_create_ch_edit_fullsreeen(void);
extern int32_t app_play_id_get(void);
static void app_channel_list_get_data_font(void);

ChannelList s_Cl = {
    .first_in = 0,
    .mode = LIST_MODE_PROG,
    .id_ops = {NULL},
    .init = channel_list_init
};

int app_get_channel_list_mode(void)
{
    return s_Cl.mode;
}

static inline void _show_group(char* str)
{
    if (str == NULL)
    {
        GUI_SetProperty(TXT_GROUP,  "state", "hide");
        GUI_SetProperty(CMB_GROUP,  "state", "show");
        GUI_SetProperty(IMG_BOTTOM, "state", "show");
        GUI_SetProperty(TXT_TP,     "state", "show");

        GUI_SetProperty(IMG_RED,    "state", "show");
        GUI_SetProperty(TXT_RED,    "state", "show");
        GUI_SetProperty(IMG_GREEN,  "state", "show");
        GUI_SetProperty(TXT_GREEN,  "state", "show");
        GUI_SetProperty(IMG_YELLOW, "state", "show");
        GUI_SetProperty(TXT_YELLOW, "state", "show");
    }
    else
    {
        GUI_SetProperty(CMB_GROUP,  "state", "hide");
        GUI_SetProperty(TXT_TP,     "state", "hide");

        GUI_SetProperty(IMG_RED,    "state", "hide");
        GUI_SetProperty(TXT_RED,    "state", "hide");
        GUI_SetProperty(IMG_GREEN,  "state", "hide");
        GUI_SetProperty(TXT_GREEN,  "state", "hide");
        GUI_SetProperty(IMG_YELLOW, "state", "hide");
        GUI_SetProperty(TXT_YELLOW, "state", "hide");

        GUI_SetProperty(IMG_BOTTOM, "state", "hide");

        GUI_SetProperty(TXT_GROUP,  "string", str);
        GUI_SetProperty(TXT_GROUP,  "state", "show");
    }
}

static uint8_t *_skip_ctrl_char(uint8_t *p, int32_t len)
{
    int32_t i = 0;

    if(NULL == p || len <= 0)
        return NULL;

    for(i = 0; i < len; i++)
    {
        if(p[i] >= ' ') //空格字符前都是控制字符
            return p + i;
    }

    return NULL;
}

void prog_list_find_program(char *strFind)
{
    s_prog_list_find_num = 0;
    APP_FREE(s_prog_list_find_id_array);

    GxMsgProperty_NodeNumGet prog_num = {0};
    prog_num.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);

    if(prog_num.node_num == 0)
        return;

    uint32_t *id_array = NULL;
    id_array = (uint32_t *)GxCore_Calloc(prog_num.node_num, sizeof(uint32_t));
    if(id_array == NULL)
    {
        app_log_error("[err]malloc failed!!!-----[%s:%d]-----\n", __FILE__, __LINE__);
        return;
    }

    uint8_t *p = NULL;
    int i = 0;
    GxMsgProperty_NodeByPosGet node_prog = {0};
    for(i = 0; i < prog_num.node_num; i++)
    {
        memset(&node_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_prog.node_type = NODE_PROG;
        node_prog.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

        p = _skip_ctrl_char(node_prog.prog_data.prog_name, MAX_PROG_NAME);

        if(strFind == NULL || strlen(strFind) == 0 || ((p != NULL) && (strncasecmp((char*)p, strFind, strlen(strFind)) == 0)))
        {
            if(g_AppChOps.get_visible_flag(i) == true)
                id_array[s_prog_list_find_num++] = node_prog.prog_data.id;
        }
    }

    if(s_prog_list_find_num == 0)
    {
        APP_FREE(id_array);
        return;
    }

    s_prog_list_find_id_array = (uint32_t *)GxCore_Calloc(s_prog_list_find_num, sizeof(uint32_t));
    if(s_prog_list_find_id_array == NULL)
    {
        app_log_error("[err]malloc failed!!!-----[%s:%d]-----\n", __FILE__, __LINE__);
        s_prog_list_find_num = 0;
        APP_FREE(id_array);
        return;
    }
    memcpy(s_prog_list_find_id_array, id_array, s_prog_list_find_num * sizeof(uint32_t));
    APP_FREE(id_array);
    return;
}

static void find_keyboard_release_cb(PopKeyboard *data)
{
    if((data->in_ret == POP_VAL_CANCEL) || (data->out_name == NULL) || (s_prog_list_find_num == 0))
    {
        GUI_EndDialog(WND_CHANNEL_LIST);
        GUI_SetProperty(WND_CHANNEL_LIST,"draw_now",NULL);
        app_channel_info_create_dialog();
    }
}

static void find_keyboard_change_cb(PopKeyboard *data)
{
    int cur_sel = 0;
    prog_list_find_program(data->out_name);
    GUI_SetProperty(LV_LIST, "update_all", NULL);
    GUI_SetProperty(LV_LIST, "select", (void*)(&cur_sel));
    GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "update", NULL);
}

#define FIND_LIST
static void find_keyboard_create(void)
{
    static PopKeyboard keyboard;

    app_keyboard_save_cb(app_input_save_space_cb);
    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num    = MAX_PROG_NAME - 1;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = find_keyboard_change_cb;
    keyboard.release_cb = find_keyboard_release_cb;
    keyboard.usr_data   = NULL;
    keyboard.pos.x      = APP_WND_KB_IN_POS_X;
    keyboard.pos.y      = APP_WND_KB_IN_POS_Y;
    multi_language_keyboard_create(&keyboard);
}

static void find_ok_keypress(void)
{
    int sel = 0;
    GUI_GetProperty(LV_LIST, "select", &sel);
    GxMsgProperty_NodeByIdGet node_prog;

    if(s_prog_list_find_id_array == NULL)
    {
        GUI_EndDialog(WND_CHANNEL_LIST);
        GUI_SetProperty(WND_CHANNEL_LIST,"draw_now",NULL);
        app_channel_info_create_dialog();
        return;
    }
    node_prog.node_type = NODE_PROG;
    node_prog.id = s_prog_list_find_id_array[sel];
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node_prog));
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_switch_tip_get_by_prog(&node_prog.prog_data) < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_switch_tip_get_by_prog(&node_prog.prog_data) < 0)
        return;
#endif
    GUI_EndDialog(WND_CHANNEL_LIST);

    if(((node_prog.prog_data.service_type == GXBUS_PM_PROG_TV)
                && (node_prog.prog_data.id == g_AppPlayOps.current_play.view_info.tv_prog_cur))
            || ((node_prog.prog_data.service_type == GXBUS_PM_PROG_RADIO)
                && (node_prog.prog_data.id == g_AppPlayOps.current_play.view_info.radio_prog_cur)))

    {
        if((g_AppFullArb.state.pause == STATE_ON || g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
                && g_AppPvrOps.state == PVR_DUMMY)
        {
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            app_play_current_prog(PLAY_MODE_POINT);
        }
        else
        {
            GUI_SetProperty(WND_CHANNEL_LIST,"draw_now",NULL);
            app_channel_info_create_dialog();
        }
        return;
    }

    if(node_prog.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        int service_id, frequency, symbol_rate, i;

        GxMsgProperty_NodeByIdGet node_tp = {0};
        node_tp.node_type = NODE_TP;
        node_tp.id = node_prog.prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

        service_id = node_prog.prog_data.service_id;
        frequency = node_tp.tp_data.frequency;
        symbol_rate = node_tp.tp_data.tp_s.symbol_rate;

        GxMsgProperty_NodeByPosGet node;
        for(i=0; i<g_AppPlayOps.normal_play.play_total; i++)
        {
            memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
            node.node_type = NODE_PROG;
            node.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

            memset(&node_tp, 0, sizeof(GxMsgProperty_NodeByIdGet));
            node_tp.node_type = NODE_TP;
            node_tp.id = node.prog_data.tp_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

            if(node.prog_data.service_id == service_id
                    && node_tp.tp_data.frequency == frequency
                    && node_tp.tp_data.tp_s.symbol_rate == symbol_rate)
            {
                g_AppPlayOps.program_play(PLAY_MODE_POINT, i);
                break;
            }
        }
    }
    else
    {
        if(app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, node_prog.prog_data.id, &g_AppPlayOps.normal_play.play_count) == GXCORE_ERROR)
        {
            g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(0);
        }
        app_play_current_prog(PLAY_MODE_POINT);
    }

    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    return;
}
//根据当前view_info找到现在是第几个组
int channel_list_get_cur_group_num(void)
{
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    return channel_get_cur_group_num(cl_id);
}

static int channel_list_get_valid_group_id(void)
{
    status_t ret = GXCORE_SUCCESS;
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    AppIdOps *fav_id = s_Cl.id_ops[LIST_MODE_FAV];
    AppIdOps *hd_id = s_Cl.id_ops[LIST_MODE_HD];

#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
	AppIdOps *sat_id = s_Cl.id_ops[LIST_MODE_SAT];
    if ((sat_id == NULL) || (cl_id == NULL) || (fav_id == NULL) || (hd_id == NULL))
        return GXCORE_ERROR;
    ret = channel_get_valid_group_id(cl_id, sat_id, fav_id, hd_id, VIEW_INFO_SKIP_VANISH);
#else
	if ((cl_id == NULL) || (fav_id == NULL)|| (hd_id == NULL))
        return GXCORE_ERROR;
    ret = channel_get_valid_group_id(cl_id, NULL, fav_id, hd_id, VIEW_INFO_SKIP_VANISH);
#endif
    return ret;
}

#if WEBLET_SUPPORT
int app_weblet_switch_cmb_group(GroupDir dir)
{
    uint32_t id_total = 0;
    uint32_t *id_buffer = NULL;
    int sel = 0;
    int i=0;
    static GxMsgProperty_NodeByPosGet ProgNode;

    for (sel=0; sel<LIST_MODE_TOTAL; sel++)
    {
        if( s_Cl.id_ops[sel] != NULL)
        {
            del_id_ops(s_Cl.id_ops[sel]);
            s_Cl.id_ops[sel] = NULL;
        }
        s_Cl.id_ops[sel] = new_id_ops();
    }

    channel_list_get_valid_group_id();
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer);
    sel = channel_list_get_cur_group_num();

    if(dir == NEXT_GROUP)
    {
        sel++;
        if(sel >= id_total)
            sel = 0;
    }
    else
    {
        sel--;
        if(sel<0)
        sel=id_total-1;
    }
    uint32_t old_id = 0;

    if (s_Cl.first_in == 0)
    {
        GxBusPmDataProgType type = g_AppPlayOps.normal_play.view_info.stream_type;
        uint32_t tv_id = g_AppPlayOps.normal_play.view_info.tv_prog_cur;
        uint32_t radio_id = g_AppPlayOps.normal_play.view_info.radio_prog_cur;
        old_id = (type == GXBUS_PM_PROG_TV) ? tv_id : radio_id;
    }

    if(0 == sel)//all satellite
    {
        g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
    }
    else//sat or fav
    {
        if((id_buffer[sel] & FAV_MASK) == 0)//sat
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
            g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
        }
        else if(id_buffer[sel] == HD_MASK)
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_HD;
        }
        else//fav
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_FAV;
            g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & (~FAV_MASK));
        }
    }

    g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
    ProgNode.node_type = NODE_PROG;
    ProgNode.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

    for (i=0; i<LIST_MODE_TOTAL; i++)
    {
        del_id_ops(s_Cl.id_ops[i]);
        s_Cl.id_ops[i] = NULL;
    }
    s_Cl.mode = LIST_MODE_PROG;

    // first in (lock) program needn't play again
    if (s_Cl.first_in == 1)
    {
        s_Cl.first_in = 0;
    }
    else
    {
        if(old_id != ProgNode.prog_data.id)
        {
            if(GUI_CheckDialog(WND_MAIN_MENU)!=GXCORE_SUCCESS)//weblet switch with play only fullscreen
            {
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                app_play_current_prog(PLAY_MODE_POINT);
            }
        }
        else
        {
            extern uint32_t app_play_id_set(uint32_t id);
            app_play_id_set(old_id);
        }
    }

    GxCore_Free(id_buffer);
    return 0;
}
#endif

static int _channel_list_cmb_group_build(void)
{
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    char *buffer_all = NULL;
    if(GXCORE_ERROR == channel_cmb_group_content_dump(cl_id, &buffer_all))
    {
        app_log_error("----channel_cmb_group_content_dump error, %s:%d----\n",__func__,__LINE__);
        return GXCORE_ERROR;
    }

    GUI_SetProperty("cmb_channel_list_group","content",buffer_all);
    GxCore_Free(buffer_all);

    return GXCORE_SUCCESS;
}

static void _channel_list_list_change(void)
{
#define TP_LEN	50
    uint32_t sel=0;
    static GxMsgProperty_NodeByPosGet ProgNode;
    char buffer[TP_LEN];
#if DOUBLE_S2_SUPPORT
    char BufferTemp[TP_LEN];
    char* tuner[2] = {"T1, ","T2, "};
#endif
    if (s_Cl.mode == LIST_MODE_PROG || s_Cl.mode == LIST_MODE_FIND)
    {
        GUI_GetProperty(LV_LIST, "select", &sel);
        //get prog node
        ProgNode.node_type = NODE_PROG;
        sel = g_AppChOps.real_pos(sel);
        ProgNode.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

        channel_prog_tp_info_str_get(&ProgNode.prog_data, buffer, TP_LEN);

#if DOUBLE_S2_SUPPORT
        memset(BufferTemp, 0 ,TP_LEN);
        // net search ProgNode.prog_data.tuner = 2 over tuner[2]
        if (ProgNode.prog_data.tuner<2){
            sprintf(BufferTemp,"%s",tuner[ProgNode.prog_data.tuner]);
        }
        else{
            sprintf(BufferTemp,"%s"," ");
        }

        strcat(BufferTemp,buffer);
        memcpy(buffer,BufferTemp,TP_LEN);
#endif
        GUI_SetProperty(TXT_TP,"string",buffer);
    }

    if ( APP_IS_NOT_THEME_CLASSIC_DARK ){
        app_channel_list_get_data_font();
    }
}

int prog_list_init(ChannelList *list, ListMode mode)
{
    int sel = 0;
    int active_sel = -1;

    _show_group(NULL);
    list->mode = mode;

    //all satellite
    GUI_SetProperty(LV_LIST, "update_all", NULL);
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
    GUI_SetProperty(LV_LIST, "select", &sel);
    active_sel = sel;
    GUI_SetProperty(LV_LIST, "active", &active_sel);

    //init cmb select
    sel = channel_list_get_cur_group_num();
    GUI_SetProperty("cmb_channel_list_group", "select", &sel);
    GUI_SetFocusWidget(LV_LIST);

    GxMsgProperty_NodeByPosGet node_prog_tmp = {0};
    node_prog_tmp.node_type = NODE_PROG;
    node_prog_tmp.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
    if((node_prog_tag.prog_data.id != node_prog_tmp.prog_data.id)
            || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
            || (g_AppFullArb.state.pause == STATE_ON))
    {
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        app_play_current_prog(PLAY_MODE_POINT);
    }

    return GXCORE_SUCCESS;
}

int prog_find_init(ChannelList *list, ListMode mode)
{
    _show_group(STR_ID_FIND);
    list->mode = mode;
    prog_list_find_program(NULL);

    return GXCORE_SUCCESS;
}

int sat_fav_list_init(ChannelList *list, ListMode mode)
{
    int sel = -1;
    int i = 0;
    int active_sel = -1;

#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
	GxMsgProperty_NodeByIdGet node = {0};

    if (mode == LIST_MODE_SAT)
        _show_group(STR_ID_SATELLITE);
    else
#endif
        _show_group(STR_ID_FAV);

    list->mode = mode;

    AppIdOps *id_ops = list->id_ops[list->mode];
    uint32_t id_total = 0;
    uint32_t *id_buffer = NULL;
    if(channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer) == GXCORE_ERROR)
    {
        app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
        return GXCORE_ERROR;
    }

    // get_cur_group_select
    if (list->mode == LIST_MODE_FAV)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_FAV) //fav
        {
            for(i=1; i<id_total; i++)
            {
                if((id_buffer[i] & FAV_MASK) != 0)//fav
                {
                    if(g_AppPlayOps.normal_play.view_info.fav_id == (id_buffer[i] & (~FAV_MASK)))
                    {
                        sel = i;
                        break;
                    }
                }
            }
        }

        s_cur_fav_sel = sel;
    }
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
    else if(list->mode == LIST_MODE_SAT)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_SAT) //all or fav
        {
            sel = 0;
        }
        else //sat
        {
            //get the node.prog_data.sat_id, when in NODE_PROG mode
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

            for(i=0;i<id_total;i++)
            {
                if((node.prog_data.sat_id == (id_buffer[i] & (~FAV_MASK))) && (id_buffer[i] != ALL_SAT))
                {
                    sel = i;
                    break;
                }
            }

        }
        s_cur_sat_sel = sel;
    }
#endif
    GxCore_Free(id_buffer);

    GUI_SetFocusWidget(LV_LIST);
    GUI_SetProperty(LV_LIST, "update_all", NULL);
    active_sel = sel;
    if(active_sel < 0)
        active_sel = 0;

    GUI_SetProperty(LV_LIST, "active", &active_sel);
    if(sel < 0)
        sel = 0;
    GUI_SetProperty(LV_LIST, "select", &sel);

    return GXCORE_SUCCESS;
}

static status_t prog_list_get_data(ChannelList *list, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
    ListItemPara* item = NULL;
    static GxMsgProperty_NodeByIdGet node;
    status_t ret = GXCORE_ERROR;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    if (s_Cl.mode == LIST_MODE_FIND)
	 {
        if (s_prog_list_find_id_array == NULL || item->sel >= s_prog_list_find_num)
            return GXCORE_ERROR;
        node.node_type = NODE_PROG;
        node.id = s_prog_list_find_id_array[item->sel];
        ret = app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    }
    else
    {
	    node.node_type = NODE_PROG;
	    node.id = g_AppChOps.get_id(item->sel);
	    //get node
	    ret = app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    }

    if(GXCORE_ERROR == ret)
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    item->string = app_channel_num_str_get(&node, item->sel);

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

    item->string = (char*)node.prog_data.prog_name;

    //col-2: $
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        item->image = "s_icon_money";
    else
        item->image = NULL;

    //col-3: lock
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        item->image = "s_icon_lock";
    else
        item->image = NULL;

    //col-4: hd
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;

    if ((g_AppPvrOps.state == PVR_RECORD || g_AppPvrOps.state == PVR_TMS_AND_REC) && node.prog_data.id == g_AppPvrOps.env.prog_id)
    {
        item->image = "s_ts_red";
    }
    else
    {
        if (node.prog_data.definition == GXBUS_PM_PROG_HD)
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

static status_t sat_fav_get_data(ChannelList *list, void *usrdata)
{
    ListItemPara* item = NULL;
    static GxMsgProperty_NodeByIdGet node;
    AppIdOps *id_ops = list->id_ops[list->mode];
    uint32_t id_total = 0;
    uint32_t *id_buffer = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    if(channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer) == GXCORE_ERROR)
    {
        app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
        return GXCORE_ERROR;
    }

    if (id_buffer[item->sel] == ALL_SAT)
    {
        //col-0: num
        item->x_offset = 0;
        item->image = NULL;
        item->string = NULL;

        //col-1: sat name
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        item->string = STR_ID_ALL;
    }
    else
    {
        if (list->mode == LIST_MODE_FAV)
        {
            node.node_type = NODE_FAV_GROUP;
            node.id = id_buffer[item->sel]&(~FAV_MASK);
            if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
                goto err;

            //col-0: num
            item->x_offset = 0;
            item->image = NULL;
            item->string = NULL;

            //col-1: fav name
            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = (char*)node.fav_item.fav_name;
        }
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
        else if (list->mode == LIST_MODE_SAT)
        {
            node.node_type = NODE_SAT;
            node.id = id_buffer[item->sel];
            if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
                goto err;

            //col-0: num
            item->x_offset = 0;
            item->image = NULL;
#if DOUBLE_S2_SUPPORT
            static char tuner[4];
            memset(tuner, 0, sizeof(tuner));
            snprintf(tuner, sizeof(tuner), "T%d", node.sat_data.tuner + 1);
            item->string = tuner;
#else
            item->string = NULL;
#endif

            //col-1: sat name
            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = (char *)app_get_sat_name(&node.sat_data);
        }
#endif
    }

    GxCore_Free(id_buffer);
    id_buffer = NULL;

    return GXCORE_SUCCESS;
err:
    APP_FREE(id_buffer);
    return GXCORE_ERROR;
}

void channel_list_init(ChannelList *list, ListMode mode)
{
    if(list->mode == mode)
    {
        // inorder app_channel_list_group_change not do play, fix passwd problem
        s_Cl.first_in = 1;
    }
    switch(mode)
    {
        case LIST_MODE_PROG:
            prog_list_init(list, mode);
            break;
        case LIST_MODE_FIND:
            prog_find_init(list, mode);
            break;
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
        case LIST_MODE_SAT:
#endif
        case LIST_MODE_FAV:
            sat_fav_list_init(list, mode);
            break;
        default:
            break;
    }
}

void sat_fav_keypress(ChannelList *list, uint32_t key, int sel)
{

    switch (key)
    {
        case STBK_OK:
            {
                if(sel < 0)
                    return;

#if SAT2IP_SERVER_SUPPORT
                if (app_sat2ip_switch_tip_get_force() < 0)
                    return;
#endif
#if DVB2IP_SERVER_SUPPORT
                if (app_dvb2ip_switch_tip_get_force() < 0)
                    return;
#endif
                memset(&node_prog_tag, 0, sizeof(GxMsgProperty_NodeByPosGet));
                node_prog_tag.node_type = NODE_PROG;
                node_prog_tag.pos = g_AppPlayOps.normal_play.play_count;
                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tag);

                AppIdOps *id_ops = list->id_ops[list->mode];
                uint32_t id_total = 0;
                uint32_t *id_buffer = NULL;
                if(channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer) == GXCORE_ERROR)
                {
                    app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
                    return;
                }

                GxBusPmViewInfo view_info;
                app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
                if (id_buffer[sel] == ALL_SAT)
                {
                    view_info.group_mode = GROUP_MODE_ALL;	//all
                }
                else
                {
                    if(id_buffer[sel] == HD_MASK)
                    {
                        view_info.group_mode = GROUP_MODE_HD;
                    }
                    else if((id_buffer[sel] & FAV_MASK) == 0) //sat
                    {
                        view_info.group_mode = GROUP_MODE_SAT;
                        view_info.sat_id = id_buffer[sel];
                    }
                    else//fav
                    {
                        view_info.group_mode = GROUP_MODE_FAV;
                        view_info.fav_id = id_buffer[sel]&(~FAV_MASK);
                    }
                }

                GxCore_Free(id_buffer);

                g_AppPlayOps.play_list_create(&(view_info));
                list->init(list, LIST_MODE_PROG);
                break;
            }

        case STBK_MENU:
        case STBK_EXIT:
            list->init(list, LIST_MODE_PROG);
            GUI_SetProperty(LV_LIST, "update_all", NULL);
            break;
        default:
            break;
    }
    // inorder to refresh tp info
    _channel_list_list_change();
}

//under the fav or sat list mode, get the program in which sat or fav group
static int _sav_fav_list_get_cur_list_sel(ChannelList *list, ListMode mode)
{
    int sel = 0;
    AppIdOps *id_ops = NULL;
    uint32_t id_total = 0;
    uint32_t *id_buf = NULL;
    int i = 0;

#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
	GxMsgProperty_NodeByIdGet node = {0};
    if (mode == LIST_MODE_SAT)
        _show_group(STR_ID_SATELLITE);
    else if(mode == LIST_MODE_FAV)
#else
	if(mode == LIST_MODE_FAV)
#endif
        _show_group(STR_ID_FAV);
    list->mode = mode;
    id_ops = list->id_ops[list->mode];

    // get_cur_group_select
    if (list->mode == LIST_MODE_FAV)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_FAV) //fav
        {
            if((id_total=id_ops->id_total_get(id_ops)) != 0)
            {
                id_buf = GxCore_Malloc(id_total*sizeof(uint32_t));
                if (id_buf != NULL)
                {
                    id_ops->id_get(id_ops, id_buf);
                    //get the node.prog_data.favorite_flag , when in NODE_PROG mode
                    //no need to find node by pos ,because one prog maybe have two or more fav group.
                    for(i=1; i<id_total; i++)
                    {
                        if((id_buf[i] & FAV_MASK) != 0)//fav
                        {
                            if(g_AppPlayOps.normal_play.view_info.fav_id == (id_buf[i] & (~FAV_MASK)))
                            {
                                sel = i;
                                break;
                            }
                        }
                    }
                    GxCore_Free(id_buf);
                    id_buf = NULL;
                }
            }
        }

        s_cur_fav_sel = sel;
    }
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
    else if(list->mode == LIST_MODE_SAT)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_SAT) //all or fav
        {
            sel = 0;
        }
        else //sat
        {
            if((id_total=id_ops->id_total_get(id_ops)) != 0)
            {
                id_buf = GxCore_Malloc(id_total*sizeof(uint32_t));
                if (id_buf != NULL)
                {
                    id_ops->id_get(id_ops, id_buf);
                    //get the node.prog_data.sat_id, when in NODE_PROG mode
                    node.node_type = NODE_PROG;
                    node.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

                    for(i=0;i<id_total;i++)
                    {
                        if (id_buf[i] == ALL_SAT)
                            ;
                        else if((id_buf[i]&(~FAV_MASK)) == node.prog_data.sat_id)
                        {
                            sel = i;
                            break;
                        }
                    }

                    GxCore_Free(id_buf);
                    id_buf = NULL;
                }
            }
        }
        s_cur_sat_sel = sel;
    }
#endif
    return sel;
}

int app_channel_list_get_cur_list_sel(void)
{
    int sel = 0;
    int prog_id = 0;

    for (sel=0; sel<LIST_MODE_TOTAL; sel++)
    {
        if( s_Cl.id_ops[sel] != NULL)
        {
            del_id_ops(s_Cl.id_ops[sel]);
            s_Cl.id_ops[sel] = NULL;
        }
        s_Cl.id_ops[sel] = new_id_ops();
    }
    channel_list_get_valid_group_id();
    if(LIST_MODE_PROG == app_get_channel_list_mode())
    {
        _channel_list_cmb_group_build();

        prog_id = app_play_id_get();
        if(GXCORE_ERROR == app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info,
                    prog_id, &sel))
        {
            g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(0);
        }
    }
    else
    {
        sel = _sav_fav_list_get_cur_list_sel(&s_Cl, app_get_channel_list_mode());
    }

    return sel;
}

static void app_channel_list_get_data_font(void)
{
    int select = 0, i = 0;
    int index = 0;
    GuiWidget *value = NULL;

    char *item_name[12] = {
        "listitem_channel_1",
        "listitem_channel_2",
        "listitem_channel_3",
        "listitem_channel_4",
        "listitem_channel_5",
        "listitem_channel_6",
        "listitem_channel_7",
        "listitem_channel_8",
        "listitem_channel_9",
        "listitem_channel_10"
    };

    for(i = 0; i <10; i++)
    {
        GUI_SetProperty(item_name[i], "font", "roboto");
    }


    for(index = 0; index<10;index++)
    {
        GUI_GetProperty(LV_LIST, "focus_widget", &value);

        if((value&&(value->name!=NULL)) && (0 == strcmp(value->name, item_name[index])))
        {
            select = index;
            break;
        }

    }

    GUI_SetProperty(item_name[select], "font", "font_prog");//font_mid
    return;
}

SIGNAL_HANDLER int app_channel_list_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
    int active_sel = -1;

    for (sel=0; sel<LIST_MODE_TOTAL; sel++)
    {
        if( s_Cl.id_ops[sel] != NULL)
        {
            del_id_ops(s_Cl.id_ops[sel]);
            s_Cl.id_ops[sel] = NULL;
        }
        s_Cl.id_ops[sel] = new_id_ops();
    }

    //-- prepare valid group id & commonbox item , before any list init --//
    channel_list_get_valid_group_id();
    _channel_list_cmb_group_build();
    //--prepare end--//

    // TODO shenbin !!!!

    //init cmb select
    sel = channel_list_get_cur_group_num();
    if (sel != 0)
    {
        // sel=0 all satellite, won't call app_channel_list_group_change, so keep this 0
        s_Cl.first_in = 1;
        GUI_SetProperty("cmb_channel_list_group", "select", &sel);
    }
    else
    {
        GUI_SetProperty(LV_LIST, "update_all", NULL);
        sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
        GUI_SetProperty(LV_LIST, "select", &sel);
        if(g_AppChOps.get_visible_flag(g_AppPlayOps.normal_play.play_count) == true)
        {
            active_sel = sel;
            GUI_SetProperty(LV_LIST, "active", &active_sel);
        }
    }
    _channel_list_list_change();
    GUI_SetFocusWidget(LV_LIST);
    if( APP_IS_NOT_THEME_CLASSIC_DARK ) {
        app_channel_list_get_data_font();
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_destroy(GuiWidget *widget, void *usrdata)
{
    uint32_t i = 0;

    s_prog_list_find_num = 0;
    APP_FREE(s_prog_list_find_id_array);
    for (i=0; i<LIST_MODE_TOTAL; i++)
    {
        del_id_ops(s_Cl.id_ops[i]);
        s_Cl.id_ops[i] = NULL;
    }
    s_Cl.mode = LIST_MODE_PROG;
    if (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_PASSWD);
    }
    return EVENT_TRANSFER_STOP;
}

static void _ch_list_pop_sort_exit(PopDlgRet ret, uint32_t pos)
{
    uint32_t sel;
    if(ret == POP_VAL_OK)
    {
        GUI_SetProperty(LV_LIST, "update_all", NULL);
        GUI_SetProperty(LV_LIST, "select", &pos);
        GUI_SetProperty(LV_LIST, "active", &pos);
        g_AppPlayOps.normal_play.play_count = g_AppChOps.real_pos(pos);
        g_AppPlayOps.current_play= g_AppPlayOps.normal_play;
        if(g_AppFullArb.state.pause == STATE_ON)
        {
            g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        }
    }
    else
    {
        sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
        GUI_SetProperty(LV_LIST, "select", &sel);
        GUI_SetProperty(LV_LIST, "active", &sel);

    }
}

static int _stop_pvr_ok_keypress(pvr_state cur_pvr)
{
    uint32_t i = 0, sel = 0,play_count = 0;
    GxMsgProperty_NodeByPosGet node_prog_tmp = {0};

    GUI_GetProperty(LV_LIST, "select", &sel);

    node_prog_tmp.node_type = NODE_PROG;
    node_prog_tmp.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);

    if (s_Cl.mode == LIST_MODE_PROG)
    {
        play_count = g_AppChOps.real_pos(sel);
        if((cur_pvr != PVR_DUMMY)
                || (node_prog_tag.prog_data.id != node_prog_tmp.prog_data.id)
                || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
                || (g_AppFullArb.state.pause == STATE_ON))
        {
            if(cur_pvr == PVR_RECORD)
            {
                GUI_SetProperty(LV_LIST,"update_all",NULL);
                for(i = 0; i < g_AppPlayOps.normal_play.play_total; i++)
                {
                    node_prog_tmp.node_type = NODE_PROG;
                    node_prog_tmp.pos = i;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
                    if(node_prog_tmp.prog_data.id == node_prog_tag.prog_data.id)
                    {
                        play_count = i;
                        sel = g_AppChOps.real_select(i);
                        break;
                    }
                }
            }
            GUI_SetProperty(LV_LIST, "select", &sel);
            GUI_SetProperty(LV_LIST, "active", &sel);
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            g_AppPlayOps.program_play(PLAY_MODE_POINT, play_count);
        }
        else
        {
            GUI_EndDialog(WND_CHANNEL_LIST);
            GUI_SetProperty(WND_CHANNEL_LIST,"draw_now",NULL);
            app_channel_info_create_dialog();
        }
    }
    else
    {
        sat_fav_keypress(&s_Cl, STBK_OK, sel);

        node_prog_tag.node_type = NODE_PROG;
        node_prog_tag.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tag);

        if(cur_pvr != PVR_DUMMY && node_prog_tag.prog_data.id == node_prog_tmp.prog_data.id)
            app_play_current_prog(PLAY_MODE_POINT);
    }
    return 0;
}

static int _stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if (POP_VAL_OK == ret)
    {
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _stop_pvr_ok_keypress(cur_pvr);
    }
    return 0;
}

static int _channel_list_list_get_total(void)
{
    ChannelList *list = &s_Cl;
    AppIdOps *id_ops = list->id_ops[list->mode];
    uint32_t total = 0;

    if(id_ops != NULL)
    {
        if (list->mode == LIST_MODE_PROG)
        {
            total = g_AppChOps.get_list_total();
        }
        else if (list->mode == LIST_MODE_FIND)
        {
            total = s_prog_list_find_num;
        }
        else
        {
            total = id_ops->id_total_get(id_ops);
        }
    }
    return total;
}

SIGNAL_HANDLER int app_channel_list_keypress(GuiWidget *widget, void *usrdata)
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
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
                case STBK_SAT:
                    if ( app_fuse_spec_sat_key_is_redefine() )
                        break;
                    if(s_Cl.mode == LIST_MODE_SAT)
                    {
                        break;
                    }
                    GUI_SetProperty(WND_CHANNEL_LIST, "state", "hide");
                    if (s_Cl.mode != LIST_MODE_SAT)
                    {
                        s_Cl.init(&s_Cl, LIST_MODE_SAT);
                    }
                    GUI_SetInterface("flush",NULL);
                    GUI_SetProperty(WND_CHANNEL_LIST, "state", "show");
                    GUI_SetFocusWidget(LV_LIST);
                    break;
#endif
                case STBK_FAV:
                    {
                        AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
						AppIdOps *hd_id = s_Cl.id_ops[LIST_MODE_HD];
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
                        AppIdOps *sat_id = s_Cl.id_ops[LIST_MODE_SAT];


                        if((cl_id == NULL) || (sat_id == NULL) || (hd_id == NULL) || s_Cl.mode == LIST_MODE_FAV)
                        {
                            break;
                        }
                        if (cl_id->id_total_get(cl_id)
                                == sat_id->id_total_get(sat_id) + hd_id->id_total_get(hd_id))
#else
						if((cl_id == NULL) || (hd_id == NULL) || s_Cl.mode == LIST_MODE_FAV)
                        {
                            break;
                        }
						if (cl_id->id_total_get(cl_id)
                                == 1 + hd_id->id_total_get(hd_id))
#endif
                        {
                            PopDlg pop;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.str = STR_ID_NO_FAV;
                            pop.timeout_sec = 2;
                            pop.mode = POP_MODE_UNBLOCK;
                            popdlg_create(&pop);
                            break;
                        }
                        GUI_SetProperty(WND_CHANNEL_LIST, "state", "hide");
                        if (s_Cl.mode != LIST_MODE_FAV)
                        {
                            s_Cl.init(&s_Cl, LIST_MODE_FAV);
                        }
                        GUI_SetInterface("flush",NULL);
                        GUI_SetProperty(WND_CHANNEL_LIST, "state", "show");
                        GUI_SetFocusWidget(LV_LIST);
                    }
                    break;
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
                case VK_BOOK_TRIGGER:
                    {
                        GUI_EndDialog(WND_CHANNEL_LIST);
                        GUI_SetInterface("flush", NULL);
                        app_channel_info_create_dialog();
                    }
                    break;
                case STBK_FIND:
                case STBK_GREEN: //find
                    if(g_AppPvrOps.state != PVR_DUMMY)
                    {
                        app_pvr_create_no_btn_popdlg(NULL);
                        break;
                    }
                    if(s_Cl.mode == LIST_MODE_FIND)
                    {
                        break;
                    }
                    s_Cl.init(&s_Cl, LIST_MODE_FIND);
                    GUI_SetInterface("flush",NULL);
                    GUI_SetFocusWidget(LV_LIST);
                    find_keyboard_create();
                    break;

                case STBK_YELLOW://edit
#if SAT2IP_SERVER_SUPPORT
                    if (app_sat2ip_operate_tip_get_force() < 0)
                        break;
#endif
#if DVB2IP_SERVER_SUPPORT
                    if (app_dvb2ip_operate_tip_get_force() < 0) //modify by gosta.wu 20220317. ->reason: first can into menu
                        break;
#endif
                    GUI_EndDialog(WND_CHANNEL_LIST);
                    GUI_SetInterface("flush", NULL);
#if ATSCCC_SUPPORT
                    extern handle_t app_atsc_unload(void);
                    app_atsc_unload();
#endif
                    app_create_ch_edit_fullsreeen();

                    if(g_AppPvrOps.state != PVR_DUMMY)
                    {
                        app_channel_info_create_dialog();
                    }
                    break;

                case STBK_RED://sort
                    if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK || APP_THEME_GRID_PURPLE ){
                    if (g_AppPvrOps.state != PVR_DUMMY)
                    {
                        app_pvr_create_no_btn_popdlg(NULL);
                        break;
                    }

#if TKGS_SUPPORT
                    if(app_check_tkgs_control_db() != 0)
                    {
                        break;
                    }
                    else
#endif
                    {
                        PopDlgPos pop_pos = {0};
                        uint32_t sel;
                        sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
                        GUI_SetProperty(LV_LIST, "select", &sel);
                        GUI_SetProperty(LV_LIST, "active", &sel);
                        app_pop_sort_create(pop_pos, g_AppPlayOps.normal_play.play_count, _ch_list_pop_sort_exit);
                        break;
                    }
                    }
                    else{
                        break;
                    }
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_channel_list_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_group_change(GuiWidget *widget, void *usrdata)
{
#define _LEN 50
    uint32_t sel = 0;
    uint32_t id_total = 0;
    uint32_t *id_buffer = NULL;
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    static GxMsgProperty_NodeByIdGet SatNode;
    static GxMsgProperty_NodeByPosGet ProgNode;
    char buffer[_LEN];
#if DOUBLE_S2_SUPPORT
    char buffertemp[_LEN];
    char* Tuner[2] = {"T1, ","T2, "};
#endif
    uint32_t old_id = 0;

    if(channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer) == GXCORE_ERROR)
    {
        app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
        return EVENT_TRANSFER_STOP;
    }

    if(id_total == 1)
    {
        GxCore_Free(id_buffer);
        return EVENT_TRANSFER_STOP;
    }

    if (s_Cl.first_in == 0)
    {
        GUI_GetProperty(LV_LIST, "active", &sel);
        old_id = g_AppChOps.get_id(sel);
        sel = 0;
    }

    GUI_GetProperty("cmb_channel_list_group", "select", &sel);
    if(0 == sel)//all satellite
    {
        g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
    }
    else//sat or fav
    {
        if(id_buffer[sel] == HD_MASK)
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_HD;
        }
        else if((id_buffer[sel] & FAV_MASK) == 0)//sat
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
            g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
        }
        else//fav
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_FAV;
            g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & (~FAV_MASK));
        }
    }

    if ( app_fuse_spec_switch_work_group_change() != 0 )
    {
        GxCore_Free(id_buffer);
        return EVENT_TRANSFER_STOP;
    }

    g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));

    //update listview show
    GUI_SetProperty(LV_LIST,"update_all",NULL);
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
    GUI_SetProperty(LV_LIST,"select",&sel);
    if(s_Cl.first_in == 1)
    {
        if(g_AppChOps.get_visible_flag(g_AppPlayOps.normal_play.play_count) == true)
        {
            GUI_SetProperty(LV_LIST, "active", &sel);
        }
    }
    else
    {
        GUI_SetProperty(LV_LIST, "active", &sel);
        g_AppPlayOps.normal_play.play_count = g_AppChOps.real_pos(sel);
    }

    //get prog node
    ProgNode.node_type = NODE_PROG;
    ProgNode.pos = g_AppChOps.real_pos(sel);
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

    channel_prog_tp_info_str_get(&ProgNode.prog_data, buffer, _LEN);
#if DOUBLE_S2_SUPPORT
    memset(buffertemp,0,_LEN);
    sprintf(buffertemp, "%s",Tuner[ProgNode.prog_data.tuner]);
    strcat(buffertemp,buffer);
    memcpy(buffer,buffertemp,_LEN);
#endif
    GUI_SetProperty(TXT_TP,"string",buffer);


    SatNode.node_type = NODE_SAT;
    SatNode.id = ProgNode.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

    memset(buffer, 0, _LEN);
    sprintf(buffer,",%s",(char*)app_get_sat_name(&SatNode.sat_data));
    GUI_SetProperty(CMB_GROUP,"string",buffer);

    // first in (lock) program needn't play again
    if (s_Cl.first_in == 1)
    {
        s_Cl.first_in = 0;
    }
    else
    {
        if(old_id != ProgNode.prog_data.id)
        {
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            app_play_current_prog(PLAY_MODE_POINT);
        }
        else
        {
            extern uint32_t app_play_id_set(uint32_t id);
            app_play_id_set(old_id);
            g_AppPlayOps.current_play = g_AppPlayOps.normal_play;
        }
    }

    GxCore_Free(id_buffer);
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_channel_list_list_change(GuiWidget *widget, void *usrdata)
{
    _channel_list_list_change();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    return _channel_list_list_get_total();
}

SIGNAL_HANDLER int app_channel_list_list_get_data(GuiWidget *widget, void *usrdata)
{
    if (s_Cl.mode == LIST_MODE_PROG || s_Cl.mode == LIST_MODE_FIND)
    {
        prog_list_get_data(&s_Cl, usrdata);
    }
    else
    {
        sat_fav_get_data(&s_Cl, usrdata);
    }

    return EVENT_TRANSFER_STOP;
}

static int _left_or_right_stop_pvr(void)
{
    GxMsgProperty_NodeByPosGet node_prog = {0};
    unsigned short pvr_prog_id = g_AppPvrOps.env.prog_id;
    unsigned short cur_prog_id = 0;
    pvr_state cur_pvr = app_pvr_get_state();

    if(cur_pvr == PVR_TIMESHIFT)
    {
        app_pvr_tms_stop();
    }
    else
    {
        app_pvr_stop();
    }

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    cur_prog_id = node_prog.prog_data.id;

    if((g_AppPvrOps.state == PVR_RECORD && cur_prog_id == pvr_prog_id) || cur_pvr == PVR_TIMESHIFT)
    {
        g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
        app_play_current_prog(PLAY_MODE_POINT);
    }

    return 0;
}

static int _left_or_right_normal(unsigned short key)
{
    static GUI_Event event = {0};
    event.type = GUI_KEYDOWN;
    event.key.sym = key;
    GUI_SendEvent("cmb_channel_list_group", &event);
    return 0;
}

static int _left_keypress_pop_cb(PopDlgRet ret)
{
    if (POP_VAL_OK == ret)
    {
        _left_or_right_stop_pvr();
        _left_or_right_normal(STBK_LEFT);
    }
    return 0;
}

static int _right_keypress_pop_cb(PopDlgRet ret)
{
    if (POP_VAL_OK == ret)
    {
        _left_or_right_stop_pvr();
        _left_or_right_normal(STBK_RIGHT);
    }
    return 0;
}

SIGNAL_HANDLER int app_channel_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t sel=0;

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
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

#if (TURN_OVER_MODE == 0)
                case STBK_LEFT:
#if SAT2IP_SERVER_SUPPORT
                    if (app_sat2ip_switch_tip_get_force() < 0)
                        break;
#endif
#if DVB2IP_SERVER_SUPPORT
                    if (app_dvb2ip_switch_tip_get_force() < 0)
                        break;
#endif
                    if (s_Cl.mode == LIST_MODE_PROG)
                    {
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_left_keypress_pop_cb))
                            _left_or_right_normal(STBK_LEFT);
                    }
                    break;

                case STBK_RIGHT:
#if SAT2IP_SERVER_SUPPORT
                    if (app_sat2ip_switch_tip_get_force() < 0)
                        break;
#endif
#if DVB2IP_SERVER_SUPPORT
                    if (app_dvb2ip_switch_tip_get_force() < 0)
                        break;
#endif
                    if (s_Cl.mode == LIST_MODE_PROG)
                    {
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_right_keypress_pop_cb))
                            _left_or_right_normal(STBK_RIGHT);
                    }
                    break;
#endif

                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    {
                        if (1)//(s_Cl.mode == LIST_MODE_PROG)
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        else
                        {
                            sat_fav_keypress(&s_Cl, event->key.sym, 0);
                        }
                    }
                    break;

                case STBK_RED:
                case STBK_GREEN:
                case STBK_YELLOW:
                    if(s_Cl.mode == LIST_MODE_PROG)
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_OK:
                    {
                        if (s_Cl.mode == LIST_MODE_FIND)
                        {
                            find_ok_keypress();
                            break;
                        }
                        if(s_Cl.mode == LIST_MODE_PROG)
                        {
                            GUI_GetProperty(LV_LIST, "select", &sel);
                            sel = g_AppChOps.real_pos(sel);
                            node_prog_tag.node_type = NODE_PROG;
                            node_prog_tag.pos = sel;
                            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tag);

                            if(((PVR_TMS_AND_REC == g_AppPvrOps.state) || (PVR_TIMESHIFT == g_AppPvrOps.state))
                                    && (node_prog_tag.prog_data.id == g_AppPvrOps.env.prog_id))
                            {
                                GUI_EndDialog(WND_CHANNEL_LIST);
                                GUI_SetInterface("flush",NULL);
                                app_channel_info_create_dialog();
                                break;
                            }
                        }

#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                        int prog_pos = 0;
                        GUI_GetProperty(LV_LIST, "select", &prog_pos);
                        prog_pos = g_AppChOps.real_pos(prog_pos);
#if SAT2IP_SERVER_SUPPORT
                        if (app_sat2ip_switch_tip_get_by_pos(prog_pos) < 0)
                            break;
#endif
#if DVB2IP_SERVER_SUPPORT
                        if (app_dvb2ip_switch_tip_get_by_pos(prog_pos) < 0)
                            break;
#endif
#endif
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_stop_pvr_cb))
                        {
                            _stop_pvr_ok_keypress(app_pvr_get_state());
                        }
                        break;
                    }

                case STBK_SAT:
                case STBK_FAV:
                    if(g_AppPvrOps.state != PVR_DUMMY && app_pvr_check_change_state())
                    {
                        app_pvr_create_no_btn_popdlg(NULL);
                        break;
                    }
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
#if TURN_OVER_MODE
                case STBK_LEFT:
#endif
                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LV_LIST, "select", &sel);
                        if (sel == 0)
                        {
                            sel = _channel_list_list_get_total() - 1;
                            GUI_SetProperty(LV_LIST, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            event->key.sym = STBK_PAGE_UP;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
#if TURN_OVER_MODE
                case STBK_RIGHT:
#endif
                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LV_LIST, "select", &sel);
                        if (_channel_list_list_get_total() == sel+1)
                        {
                            sel = 0;
                            GUI_SetProperty(LV_LIST, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            event->key.sym = STBK_PAGE_DOWN;
                            ret = EVENT_TRANSFER_KEEPON;
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

void app_channel_list_hotplug_out(uint32_t current_prog_id)
{
    if(s_Cl.mode != LIST_MODE_PROG)
    {
        return;
    }
    int sel = 0;
    app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, current_prog_id, &sel);
    sel = g_AppChOps.real_select(sel);
    GUI_SetProperty(LV_LIST, "update_all", NULL);
    GUI_SetProperty(LV_LIST, "select", &sel);
    GUI_SetProperty(LV_LIST, "active", &sel);
    GUI_SetInterface("flush", NULL);
    app_update_pop_dlg(WND_POP_TIP);
}

#if VOICE_CONTROL_SUPPORT
static void _store_matching_ids(uint32_t *pids, int *match_index, unsigned int index_count)
{
    unsigned int i = 0;
    if (s_prog_list_find_id_array != NULL)
    {
        free(s_prog_list_find_id_array);
        s_prog_list_find_id_array = NULL; // 避免悬空指针
    }

    s_prog_list_find_id_array = (uint32_t *)calloc(index_count, sizeof(uint32_t));
    if (s_prog_list_find_id_array == NULL)
    {
        app_log_error("Memory allocation failed\n");
        return;
    }

    for (i = 0; i < index_count; i++)
    {
        if (i < index_count)
        {
            s_prog_list_find_id_array[i] = pids[match_index[i]];
        }
    }
    s_prog_list_find_num = index_count;
}

unsigned int app_find_get_value_by_index(unsigned int sel,unsigned int num_elements)
{
    unsigned int channel_pos = 0;
    unsigned int channel_id = 0;

    if (s_prog_list_find_id_array == NULL)
    {
        return 0; // Assuming 0 is an invalid position or error code
    }

    if (sel > 0 && sel <= num_elements)
    {
        channel_id = s_prog_list_find_id_array[sel-1];
        if (app_get_prog_pos_by_id_cur(channel_id, (int*)&channel_pos) != 0) {
            return 0; // Assuming 0 is an invalid position
        }
    }

    if(sel > s_prog_list_find_num)
    {
        channel_pos = 0xfffe;
    }
    return (channel_pos+1);
}

void find_name_change_list(uint32_t *pids, int *match_index, unsigned int index_count)
{
    int cur_sel = 0;
    GUI_CreateDialog(WND_CHANNEL_LIST);
    s_Cl.init(&s_Cl, LIST_MODE_FIND);
    _store_matching_ids(pids, match_index, index_count);
    GUI_SetInterface("flush",NULL);
    GUI_SetFocusWidget(LV_LIST);
    GUI_SetProperty(LV_LIST, "update_all", NULL);
    GUI_SetProperty(LV_LIST, "select", (void*)(&cur_sel));
}

//ret: 0-can support; 1-not support; 2:repeat 3:the current scenario daes not support
int app_vc_channel_list_play_flag(void)
{
    if(s_Cl.mode == LIST_MODE_PROG)
    {
        return 0;
    }
    else if(s_Cl.mode == LIST_MODE_FIND)
    {
        return 0;
    }
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
    else if(s_Cl.mode == LIST_MODE_SAT)
    {
        return 3;
    }
#endif
    else
    {
        return 1;
    }
}

int app_vc_channel_list_switch_channel(void)
{
    if((s_Cl.mode == LIST_MODE_PROG)
            && (1 < _channel_list_list_get_total()))
    {
        return 0;
    }
    else if((s_Cl.mode == LIST_MODE_FIND)
            && (1 < _channel_list_list_get_total()))
    {
        return 0;
    }
    else
        return 1;

}
#endif
