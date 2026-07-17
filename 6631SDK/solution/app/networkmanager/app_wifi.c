#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_keyboard_language.h"
#include "app_book.h"
#include "app_windows.h"

#if NETWORK_SUPPORT && WIFI_SUPPORT

#ifdef ECOS_OS
bool s_wifi_set_connecting = false;
extern int app_wifi_linkdown(char* dev_name);
extern int app_get_wifi_bssid(char* dev_name,char* bssid);
#endif

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#define BUF_SMALL_LEN		33

#define LIST_WIFI						"list_wifi"
#define TXT_WIFI_LINK_STATUS			"text_wifi_link_status"
#define TXT_WIFI_LINK_TITLE			"text_wifi_link_title"
#define IMG_WIFI_LINK			"img_wifi_link"
#define TXT_WIFI_MOVE				"text_wifi_move"
#define IMG_WIFI_MOVE				"img_wifi_move"
#define TXT_WIFI_OK					"text_wifi_ok"
#define IMG_WIFI_OK					"img_wifi_ok"
#define TXT_WIFI_EXIT					"text_wifi_exit"
#define IMG_WIFI_EXIT					"img_wifi_exit"
#define TXT_WIFI_REFRESH				"text_wifi_refresh"
#define IMG_WIFI_REFRESH				"img_wifi_refresh"
#define IMG_WIFI_TIP_PSK                "img_wifi_tip_psk"
#define TXT_WIFI_BK_COLOR  "[text_wifi_bk_color,text_wifi_bk_color,text_wifi_bk_color]"
#define TXT_WIFI_FE_EN_COLOR  "[text_wifi_fe_en_color,text_wifi_fe_en_color,text_wifi_fe_dis_color]"
#define TXT_WIFI_FE_DIS_COLOR  "[text_wifi_fe_dis_color,text_wifi_fe_dis_color,text_wifi_fe_dis_color]"
#define TXT_WIFI_ESSID_COLOR  "[pop_tag_color,pop_text_color,pop_text_color]"


typedef struct _wifiap_para_t
{
    char essid[ESSID_MAX_SIZE + 1];
    char ip[BUF_SMALL_LEN];
    char psk[BUF_SMALL_LEN*2];
    char mac[BUF_SMALL_LEN];
    unsigned short encryption;
    char quality[8];
    uint8_t quality_level;
    uint8_t is_link;
    uint8_t hidden; // essid is hide
    struct _wifiap_para_t* next;
}WifiApPara;

typedef enum
{
    WIFI_MENU_SINGLE = 0,
    WIFI_MENU_ALL,
}wifiMenuMode;

typedef enum
{
    WIFI_SET_HIDE = 0,
    WIFI_SET_EDIT,
    WIFI_SET_ADD ,
}WifiSetStatus;

static wifiMenuMode s_wifi_menu_mode = WIFI_MENU_SINGLE;
static uint8_t s_dev_sel = 0;
static uint8_t s_wifi_dev_total = 0;
static int s_ap_sel= 0;
static int s_ap_total = 0;
static ubyte64 *s_wifi_dev = NULL;
static ubyte64 s_cur_dev = {0};
static uint8_t s_wifi_connect = 0;
static uint8_t s_wifi_encryption = 0;
static WifiApPara *sp_ap_head = NULL;
static WifiApPara *sp_ap_cur = NULL;
static bool s_wifi_linking = false;
static IfState s_wifi_state = IF_STATE_INVALID;
static event_list* s_wifi_init_timer = NULL;
static event_list* s_wifi_gif_timer = NULL;
static GxMsgProperty_ApList s_scan_ap_list;
static uint8_t s_wifi_set_show = WIFI_SET_HIDE;
static bool s_wifi_ap_updating = false;
static ubyte64 s_set_wifi = {0};
static WifiApPara s_set_ap;
static GxMsgProperty_CurAp s_cur_ap = {{0},};
static char *s_img_strength[] = {
	"s_signal_0",
	"s_signal_1",
	"s_signal_2",
	"s_signal_3",
	"s_signal_4",
	"s_signal_5",
};

extern char* app_network_get_state_str(IfState state);
extern int app_pop_list_end_dialog(void);
void app_wifi_dev_update(char *wifi_dev);
void app_wifi_ap_list_update(GxMsgProperty_ApList *ap_list);
void app_wifi_ap_list_clean(void);

static int app_wifi_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_wifi_gif", "draw_gif", &alu);
    return 0;
}

static void app_wifi_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_wifi_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_wifi_gif", "init_gif_alu_mode", &alu);
}

static void app_wifi_free_gif(void)
{
	GUI_SetProperty("img_wifi_gif", "load_img", NULL);
}

static void app_wifi_show_gif(void)
{
    GUI_SetProperty("img_wifi_gif", "state", "show");
    if(0 != reset_timer(s_wifi_gif_timer))
	{
		s_wifi_gif_timer = create_timer(app_wifi_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

static void app_wifi_hide_gif(void)
{
    APP_TIMER_REMOVE(s_wifi_gif_timer);
	GUI_SetProperty("img_wifi_gif", "state", "hide");
}

void ap_list_add(WifiApPara *p)
{
	if(sp_ap_head == NULL)
	{
		sp_ap_head = sp_ap_cur = p;
	}
	else
	{
		sp_ap_cur->next = p;
		sp_ap_cur = p;
	}
}

WifiApPara *ap_list_get(uint8_t index)
{
	WifiApPara *p = sp_ap_head;
	uint8_t i = 0;

	if(p == NULL)
		return p;
	for(i = 0; i < index; i++)
    {
		p = p->next;
        if(NULL == p)
        {
            if(i < index - 1)
                app_log_debug("index exceeds range\n");
            break;
        }
    }
	return p;
}

int8_t ap_list_get_sel(WifiApPara *s_ap)
{
	WifiApPara *p = sp_ap_head;
	int8_t i = 0;

	if(p == NULL)
		return -1;

    while(p != NULL)
    {
        if((strcmp(s_ap->essid, p->essid) == 0) && (strcmp(s_ap->mac, p->mac) == 0))
        {
            break;
        }
        i++;
		p = p->next;
    }

    if(p == NULL)
        i = -1;

	return i;
}

void ap_list_changeToHead(WifiApPara *p_ap)
{
	if(sp_ap_head == p_ap)
	{
		return;
	}
	else
	{
		WifiApPara *p_fap = NULL;
		WifiApPara *p_tmp = NULL;
		p_tmp = sp_ap_head;
		while(p_tmp)
		{
			if(p_tmp->next&&(p_tmp->next == p_ap))
			{
				p_fap = p_tmp;
			}
			p_tmp = p_tmp->next;
		}
		if(p_fap)
		{
			p_fap->next = p_ap->next;
			p_ap->next =  sp_ap_head;
			sp_ap_head = p_ap;
		}
	}
}

void ap_list_destroy(void)
{
	WifiApPara *p = sp_ap_head;

    if(sp_ap_head)
	{
		while(p)
		{
			sp_ap_head = p->next;
			GxCore_Free(p);
			p = sp_ap_head;
		}
	}
	sp_ap_head = sp_ap_cur = NULL;
}

void ap_list_create(GxMsgProperty_ApList *ap_list)
{
	uint8_t i =0;
	WifiApPara *p_ap = NULL;
	WifiApPara *p_cur = NULL;
    ubyte64 cur_dev;
    GxMsgProperty_CurAp cur_ap;
    GxMsgProperty_NetDevState state;
    if(ap_list->ap_num == 0)
		return;


    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, ap_list->wifi_dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    memset(&cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    strcpy(cur_ap.wifi_dev, ap_list->wifi_dev);
    app_send_msg_exec(GXMSG_WIFI_GET_CUR_AP, &cur_ap);
    for(i = 0; i < ap_list->ap_num; i++)
	{
		p_ap = (WifiApPara *)GxCore_Calloc(sizeof(WifiApPara), 1);
		ap_list_add(p_ap);

		strcpy(sp_ap_cur->mac, ap_list->ap_info[i].ap_mac);
        strcpy(sp_ap_cur->essid, ap_list->ap_info[i].ap_essid);
        sp_ap_cur->hidden = ap_list->ap_info[i].hidden;
        sprintf(sp_ap_cur->quality, "%d/100", ap_list->ap_info[i].ap_quality);

        if(ap_list->ap_info[i].ap_quality == 0)
            sp_ap_cur->quality_level = 0;
        else if(ap_list->ap_info[i].ap_quality >0 && ap_list->ap_info[i].ap_quality<=20)
            sp_ap_cur->quality_level = 1;
        else if(ap_list->ap_info[i].ap_quality >20 && ap_list->ap_info[i].ap_quality<=40)
            sp_ap_cur->quality_level = 2;
        else if(ap_list->ap_info[i].ap_quality >40 && ap_list->ap_info[i].ap_quality<=60)
            sp_ap_cur->quality_level = 3;
        else if(ap_list->ap_info[i].ap_quality >60 && ap_list->ap_info[i].ap_quality<=80)
            sp_ap_cur->quality_level = 4;
        else
            sp_ap_cur->quality_level = 5;

        sp_ap_cur->encryption = ap_list->ap_info[i].encryption;
		if(sp_ap_cur->encryption != 0)
            strcpy(sp_ap_cur->psk, ap_list->ap_info[i].ap_psk);

        if(strcmp(sp_ap_cur->mac, cur_ap.ap_mac) == 0)
        {
            memset(cur_dev, 0, sizeof(ubyte64));
            app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
            if((strcmp(cur_dev, ap_list->wifi_dev) == 0)&&(state.dev_state == IF_STATE_CONNECTED))
            {
                GxMsgProperty_NetDevIpcfg ip_cfg;
                memset(&ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
                strcpy(ip_cfg.dev_name, ap_list->wifi_dev);
                app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &ip_cfg);
                strcpy(sp_ap_cur->ip, ip_cfg.ip_addr);

                sp_ap_cur->is_link = 1;
                p_cur = sp_ap_cur;
            }
        }
#if 0
        app_log_debug("---dev:%s, ap%d: %s, %s, %d, %d, link: %d---\n", ap_list->wifi_dev, i, ap_list->ap_info[i].ap_essid, ap_list->ap_info[i].ap_mac,
                ap_list->ap_info[i].ap_quality, ap_list->ap_info[i].encryption, sp_ap_cur->is_link);
#endif
	}

	if(p_cur != NULL)
	{
		ap_list_changeToHead(p_cur);
	}
}
static bool s_wifi_tip_flag =false;
void app_wifi_tip_show(char *info_str)
{
    if(true == s_wifi_tip_flag)
        return;

	s_wifi_tip_flag = true;
    GUI_SetInterface("flush",NULL);
	GUI_SetProperty(IMG_WIFI_LINK, "state", "show");
	GUI_SetProperty(TXT_WIFI_LINK_TITLE, "state", "show");
	GUI_SetProperty(TXT_WIFI_LINK_STATUS, "state", "show");
	GUI_SetProperty(TXT_WIFI_LINK_STATUS, "string", info_str);
}

void app_wifi_tip_hide(void)
{
    if(false == s_wifi_tip_flag)
        return;

	s_wifi_tip_flag = false;
	GUI_SetProperty(IMG_WIFI_LINK, "state", "hide");
	GUI_SetProperty(TXT_WIFI_LINK_STATUS, "state", "hide");
	GUI_SetProperty(TXT_WIFI_LINK_TITLE, "state", "hide");
}

void app_wifi_tip_lost_focus_hide(void)
{
	GUI_SetProperty(IMG_WIFI_LINK, "state", "hide");
	GUI_SetProperty(TXT_WIFI_LINK_STATUS, "state", "hide");
	GUI_SetProperty(TXT_WIFI_LINK_TITLE, "state", "hide");
    GUI_SetProperty(IMG_WIFI_TIP_PSK,"state" ,"hide");
}

void app_wifi_tip_flush(void)
{
	if(s_wifi_tip_flag == true)
	{
		app_wifi_tip_hide();
		GUI_SetInterface("flush",NULL);
		GUI_SetProperty(IMG_WIFI_LINK, "state", "show");
		GUI_SetProperty(TXT_WIFI_LINK_TITLE, "state", "show");
		GUI_SetProperty(TXT_WIFI_LINK_STATUS, "state", "show");
	}
}

void app_wifi_hint_show(void)
{
	GUI_SetProperty(IMG_WIFI_OK, "state", "show");
	GUI_SetProperty(TXT_WIFI_OK, "state", "show");
	GUI_SetProperty(IMG_WIFI_MOVE, "state", "show");
	GUI_SetProperty(TXT_WIFI_MOVE, "state", "show");
	GUI_SetProperty(IMG_WIFI_EXIT, "state", "show");
	GUI_SetProperty(TXT_WIFI_EXIT, "state", "show");
	GUI_SetProperty(IMG_WIFI_REFRESH, "state", "show");
	GUI_SetProperty(TXT_WIFI_REFRESH, "state", "show");
}

void app_wifi_hint_hide(void)
{
	GUI_SetProperty(IMG_WIFI_OK, "state", "hide");
	GUI_SetProperty(TXT_WIFI_OK, "state", "hide");
	GUI_SetProperty(IMG_WIFI_MOVE, "state", "hide");
	GUI_SetProperty(TXT_WIFI_MOVE, "state", "hide");
	GUI_SetProperty(IMG_WIFI_EXIT, "state", "hide");
	GUI_SetProperty(TXT_WIFI_EXIT, "state", "hide");
	GUI_SetProperty(IMG_WIFI_REFRESH, "state", "hide");
	GUI_SetProperty(TXT_WIFI_REFRESH, "state", "hide");
}

void app_wifi_set_show(WifiSetStatus mode)
{
    if(mode == WIFI_SET_EDIT)
    {
        WifiApPara *p_ap = ap_list_get(s_ap_sel);
        memset(&s_set_ap, 0, sizeof(WifiApPara));
        if(p_ap == NULL)
            return;

        app_wifi_tip_hide();
        app_wifi_hide_gif();
        memcpy(&s_set_ap, p_ap, sizeof(WifiApPara));
        strcpy(s_set_wifi, s_wifi_dev[s_dev_sel]);

        GUI_SetProperty("img_wifi_set_bk", "state", "show");

        GUI_SetProperty("text_wifi_set0", "state", "show");
        GUI_SetProperty("text_wifi_set1", "state", "show");
        GUI_SetProperty("text_wifi_set2", "state", "show");
        GUI_SetProperty("text_wifi_set3", "state", "show");
        GUI_SetProperty("text_wifi_set4", "state", "show");
        GUI_SetProperty("button_wifi_set5", "state", "show");
        GUI_SetProperty("text_wifi_set0", "backcolor",TXT_WIFI_BK_COLOR);
        if(s_set_ap.hidden)
        {
            GUI_SetProperty("button_wifi_essid", "state", "show");
            GUI_SetProperty("button_wifi_essid", "string", s_set_ap.essid);
        }
        else
        {
            GUI_SetProperty("text_wifi_essid", "state", "show");
            GUI_SetProperty("text_wifi_essid", "string", s_set_ap.essid);
        }
        if(strlen(s_set_ap.ip) == 0)
            GUI_SetProperty("text_wifi_ip", "string", "N/A");
        else
            GUI_SetProperty("text_wifi_ip", "string", s_set_ap.ip);
        GUI_SetProperty("text_wifi_ip", "state", "show");
        GUI_SetProperty("text_wifi_mac", "string", s_set_ap.mac);
        GUI_SetProperty("text_wifi_mac", "state", "show");
        GUI_SetProperty("text_wifi_quality", "string", s_set_ap.quality);
        GUI_SetProperty("text_wifi_quality", "state", "show");
        if(s_set_ap.encryption == 0)
        {
            GUI_SetProperty("text_wifi_encryption", "string", STR_ID_OFF);
            s_wifi_encryption = 0;
        }
        else
        {
            GUI_SetProperty("text_wifi_encryption", "string", STR_ID_ON);
            s_wifi_encryption =1;
        }
        GUI_SetProperty("text_wifi_encryption", "state", "show");
        GUI_SetProperty("cmb_wifi_encryption", "state", "hide");
            GUI_SetProperty("text_wifi_set4", "forecolor",TXT_WIFI_FE_DIS_COLOR);

        if(strlen(s_set_ap.psk) == 0)
        {
            GUI_SetProperty("button_wifi_psk", "string", "N/A");
        }
        else
        {
            GUI_SetProperty("button_wifi_psk", "string", "******");
        }
        GUI_SetProperty("button_wifi_psk", "state", "show");

        if(s_set_ap.is_link == 0)
        {
            s_wifi_connect = 0;
            GUI_SetProperty("button_wifi_connect", "string", STR_ID_SAVE);
        }
        else
        {
            s_wifi_connect = 1;
            GUI_SetProperty("button_wifi_connect", "string", "Disconnect");
        }

        if(s_wifi_encryption == 1 && s_wifi_connect == 0)
        {
            GUI_SetProperty("button_wifi_set5","state", "enable");
            GUI_SetProperty("button_wifi_psk","state", "enable");
            if(s_set_ap.hidden)
            {
                GUI_SetProperty("text_wifi_set0", "forecolor", TXT_WIFI_FE_EN_COLOR);
                GUI_SetProperty("button_wifi_essid", "forecolor", TXT_WIFI_ESSID_COLOR);
            }
            else
            {
                GUI_SetProperty("text_wifi_set0", "forecolor",TXT_WIFI_FE_DIS_COLOR);
            }
        }
        else
        {
            GUI_SetProperty("button_wifi_set5","state", "disable");
            GUI_SetProperty("button_wifi_psk","state", "disable");
            GUI_SetProperty("text_wifi_set0", "forecolor",TXT_WIFI_FE_DIS_COLOR);
            if(s_set_ap.hidden)
            {
                GUI_SetProperty("button_wifi_essid", "forecolor", TXT_WIFI_FE_DIS_COLOR);
            }

        }

        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4_l");
        GUI_SetProperty("img_wifi_tip_opt", "state", "show");
        GUI_SetProperty("button_wifi_connect", "state", "show");
        GUI_SetProperty("button_wifi_cancel", "state", "show");
        GUI_SetProperty("img_wifi_tip_encryption","state","hide");
        GUI_SetProperty("img_wifi_tip_psk","state","hide");

        GUI_SetFocusWidget("button_wifi_connect");
        s_wifi_set_show = WIFI_SET_EDIT;
    }
    else if(mode == WIFI_SET_ADD)
    {
        strcpy(s_set_wifi, s_wifi_dev[s_dev_sel]);
        memset(s_set_ap.essid, 0, ESSID_MAX_SIZE + 1);
        memset(s_set_ap.ip,  0, BUF_SMALL_LEN);
        memset(s_set_ap.psk, 0, BUF_SMALL_LEN*2);
        memset(s_set_ap.mac, 0, BUF_SMALL_LEN);
        memset(s_set_ap.quality, 0, 8);
        s_set_ap.encryption = 0;;
        s_set_ap.quality_level = 0;;
        s_set_ap.is_link = 0;;
        s_set_ap.hidden = 1; // essid is hide
        s_set_ap.next = NULL;

        app_wifi_tip_hide();
        app_wifi_hide_gif();
        GUI_SetProperty("img_wifi_set_bk", "state", "show");

        GUI_SetProperty("text_wifi_set0", "state", "show");
        GUI_SetProperty("text_wifi_set0", "backcolor", "[#ff00ff,#ff00ff,#ff00ff]");
        GUI_SetProperty("text_wifi_set1", "state", "hide");
        GUI_SetProperty("text_wifi_set2", "state", "hide");
        GUI_SetProperty("text_wifi_set3", "state", "hide");
        GUI_SetProperty("text_wifi_set4", "state", "show");
        GUI_SetProperty("button_wifi_set5", "state", "show");

        GUI_SetProperty("button_wifi_essid", "state", "show");
        GUI_SetProperty("button_wifi_essid", "string", "");
        GUI_SetProperty("text_wifi_set0", "forecolor", TXT_WIFI_FE_EN_COLOR);
        GUI_SetProperty("button_wifi_essid", "forecolor", TXT_WIFI_ESSID_COLOR);

        GUI_SetProperty("text_wifi_ip", "state", "hide");
        GUI_SetProperty("text_wifi_mac", "state", "hide");
        GUI_SetProperty("text_wifi_quality", "state", "hide");

        int sel = 0;
        s_wifi_encryption = 0;
        GUI_SetProperty("cmb_wifi_encryption","content", "[Off,On]");
        GUI_SetProperty("cmb_wifi_encryption","select", &sel);
        GUI_SetProperty("text_wifi_encryption", "state", "hide");
        GUI_SetProperty("cmb_wifi_encryption", "state", "show");
        GUI_SetProperty("text_wifi_set4", "forecolor", TXT_WIFI_FE_EN_COLOR);

        GUI_SetProperty("button_wifi_psk", "string", "N/A");
        GUI_SetProperty("button_wifi_psk", "state", "show");

        s_wifi_connect = 0;
        GUI_SetProperty("button_wifi_connect", "string", STR_ID_SAVE);

        GUI_SetProperty("button_wifi_set5","state", "disable");
        GUI_SetProperty("button_wifi_psk","state", "disable");

        GUI_SetProperty("button_wifi_connect", "state", "show");
        GUI_SetProperty("button_wifi_cancel", "state", "show");
        GUI_SetProperty("img_wifi_tip_encryption","state","hide");
        GUI_SetProperty("img_wifi_tip_psk","state","hide");

        GUI_SetProperty("img_wifi_tip_essid","state","show");
        GUI_SetFocusWidget("button_wifi_essid");
        s_wifi_set_show = WIFI_SET_ADD;
    }
}

void app_wifi_set_hide(void)
{
    const char *wnd = NULL;

    wnd = GUI_GetFocusWindow();
    GUI_SetProperty("button_wifi_psk", "state", "hide");
    GUI_SetProperty("img_wifi_set_bk", "state", "hide");
    GUI_SetProperty("text_wifi_set0", "state", "hide");
    GUI_SetProperty("text_wifi_set1", "state", "hide");
    GUI_SetProperty("text_wifi_set2", "state", "hide");
    GUI_SetProperty("text_wifi_set3", "state", "hide");
    GUI_SetProperty("text_wifi_set4", "state", "hide");
    GUI_SetProperty("button_wifi_set5", "state", "hide");

    GUI_SetProperty("text_wifi_essid", "state", "hide");
    GUI_SetProperty("button_wifi_essid", "state", "hide");
    GUI_SetProperty("img_wifi_tip_essid","state", "hide");
    GUI_SetProperty("text_wifi_ip", "state", "hide");
    GUI_SetProperty("text_wifi_mac", "state", "hide");
    GUI_SetProperty("text_wifi_quality", "state", "hide");
    GUI_SetProperty("text_wifi_encryption", "state", "hide");
    GUI_SetProperty("cmb_wifi_encryption","state", "hide");
    GUI_SetProperty("button_wifi_psk", "state", "hide");
    GUI_SetProperty("img_wifi_tip_opt", "state", "hide");
    GUI_SetProperty("button_wifi_connect", "state", "hide");
    GUI_SetProperty("button_wifi_cancel", "state", "hide");
    GUI_SetProperty("img_wifi_tip_encryption","state","hide");
    GUI_SetProperty("img_wifi_tip_psk","state","hide");
    if(wnd != NULL && 0 == strcmp(wnd, WND_WIFI))
    {
        GUI_SetFocusWidget(LIST_WIFI);
    }

    s_wifi_set_show = WIFI_SET_HIDE;
}

static void app_wifi_save_ap(int hidden)
{
    if(s_wifi_dev_total ==  0)
        return;

    memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    strcpy(s_cur_ap.wifi_dev, s_wifi_dev[s_dev_sel]);
    strcpy(s_cur_ap.ap_essid, s_set_ap.essid);
    strcpy(s_cur_ap.ap_mac, s_set_ap.mac);
    strcpy(s_cur_ap.ap_psk, s_set_ap.psk);
    s_cur_ap.hidden = hidden;
    app_send_msg_exec(GXMSG_WIFI_SET_CUR_AP, &s_cur_ap);

}

static void _wifi_scan_hidden_wifi(void)
{
    GxMsgProperty_ScanHideAp param = {{0}, {0}};

    strcpy(param.dev_name, s_wifi_dev[s_dev_sel]);
    strcpy(param.ap_essid, s_set_ap.essid);
    app_send_msg_exec(GXMSG_WIFI_SCAN_HIDDEN_AP, &param);
}

void app_wifi_disconnect(void)
{
    ubyte64 wifi_name;
    if(s_wifi_dev_total > 0)
    {
        memset(wifi_name, 0, sizeof(ubyte64));
        strcpy(wifi_name, s_wifi_dev[s_dev_sel]);
        app_log_debug("[%s] del wifi %s\n", __func__, wifi_name);
        app_send_msg_exec(GXMSG_WIFI_DEL_CUR_AP, &wifi_name);
        memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    }
}

void app_wifi_ap_list_update(GxMsgProperty_ApList *ap_list)
{
    s_ap_total = 0;
    ap_list_destroy();
    ap_list_create(ap_list);
    s_ap_total = ap_list->ap_num;
}

void app_wifi_ap_list_clean(void)
{
    s_ap_total = 0;
    ap_list_destroy();
}

void app_wifi_dev_update(char *wifi_dev)
{
    ubyte64 display_name = {0};
    ubyte64 wifi_name = {0};
    GxMsgProperty_NetDevState state;
    char* state_str;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0))
        return;

    app_if_name_convert(wifi_dev, display_name);
    GUI_SetProperty("text_wifi_dev", "string", display_name);
	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strcpy(state.dev_name, wifi_dev);
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
	s_wifi_state = state.dev_state;
    state_str = app_network_get_state_str(s_wifi_state);
    GUI_SetProperty("text_wifi_status", "string", state_str);

	if(s_wifi_ap_updating)
	{
		app_wifi_tip_show(STR_ID_PLZ_WAIT);
		if(s_wifi_state == IF_STATE_CONNECTING)
        {
            app_wifi_show_gif();
        }
		return;
	}
	else
	{
		app_wifi_hide_gif();
		app_wifi_tip_hide();

		app_wifi_ap_list_clean();
		GUI_SetProperty(LIST_WIFI, "update_all", NULL);

        s_scan_ap_list.ap_num = 0;
        memset(s_scan_ap_list.wifi_dev, 0, sizeof(s_scan_ap_list.wifi_dev));
		memset(s_scan_ap_list.ap_info, 0, sizeof(ApInfo) * MAX_AP_NUM);
        if(s_wifi_state == IF_STATE_INVALID)
		{
			app_wifi_tip_show(STR_ID_NO_WIFI);
		}
		else
		{
			app_wifi_tip_show(STR_ID_PLZ_WAIT);
			memset(wifi_name, 0, sizeof(ubyte64));
			strcpy(wifi_name, wifi_dev);
			if((s_wifi_state == IF_STATE_CONNECTING) || (s_wifi_state == IF_STATE_START))
			{
				app_wifi_show_gif();
			}
			app_send_msg_exec(GXMSG_WIFI_SCAN_AP, &wifi_name);
		}
		s_wifi_ap_updating = true;
	}
}

void app_wifi_full_keyboard_proc(PopKeyboard *data)
{
    GxMsgProperty_NetDevMode dev_mode;

	if(data->in_ret == POP_VAL_CANCEL)
		return;

	if (data->out_name == NULL)
		return;

    if(strlen(data->out_name) < 8)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.format = POP_FORMAT_DLG;
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_ERR_PASSWD;
        pop.mode = POP_MODE_UNBLOCK;
#ifdef ECOS_OS
		pop.pos.x = 320;
		pop.pos.y = 196;
#endif
        popdlg_create(&pop);
        return;
    }

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    strcpy(dev_mode.dev_name, s_wifi_dev[s_dev_sel]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode == 0 && strcmp(s_set_ap.psk, data->out_name) != 0)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_OPEN_WIFI;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
        return;
    }

    memset(s_set_ap.psk, 0, BUF_SMALL_LEN*2);
    memcpy(s_set_ap.psk, data->out_name, strlen(data->out_name));

	GUI_SetProperty("button_wifi_psk", "state", "hide");
	if(strlen(s_set_ap.psk) == 0)
	{
		GUI_SetProperty("button_wifi_psk", "string", "N/A");
	}
	else
	{
		GUI_SetProperty("button_wifi_psk", "string", "******");
	}
	GUI_SetProperty("button_wifi_psk", "state", "show");
	GUI_SetFocusWidget("button_wifi_psk");
}

void app_wifi_essid_keyboard_proc(PopKeyboard *data)
{
    GxMsgProperty_NetDevMode dev_mode;

    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    strcpy(dev_mode.dev_name, s_wifi_dev[s_dev_sel]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode == 0)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_OPEN_WIFI;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
        return;
    }

    memset(s_set_ap.essid, 0, sizeof(s_set_ap.essid));
    memcpy(s_set_ap.essid, data->out_name, strlen(data->out_name));

    GUI_SetProperty("button_wifi_essid", "string", s_set_ap.essid);
    GUI_SetFocusWidget("button_wifi_essid");
}

static void app_wifi_set_exit(void)
{
    int i = 0;
    char *state_str = NULL;

    if(s_wifi_set_show)
        app_wifi_set_hide();
    if(s_wifi_dev_total > 0)
    {
        if(s_wifi_dev_total > 1)
        {
            GUI_SetProperty("img_wifi_l","state","show");
            GUI_SetProperty("img_wifi_r","state","show");
        }
        else
        {
            GUI_SetProperty("img_wifi_l","state","hide");
            GUI_SetProperty("img_wifi_r","state","hide");
        }

        if(strcmp(s_set_wifi, s_wifi_dev[s_dev_sel]) == 0) //same wifi dev
        {
            if((s_wifi_state == IF_STATE_CONNECTING) || (s_wifi_state == IF_STATE_START))
            {
                app_wifi_show_gif();
            }
            else
            {
                app_wifi_hide_gif();
            }

            for(i = 0; i < s_scan_ap_list.ap_num; i++)
            {
                if(strcmp(s_set_ap.mac, s_scan_ap_list.ap_info[i].ap_mac) == 0)
                {
                    strcpy(s_scan_ap_list.ap_info[i].ap_psk, s_set_ap.psk);
                    if(s_set_ap.hidden)
                    {
                        strcpy(s_scan_ap_list.ap_info[i].ap_essid, s_set_ap.essid);
                    }
                    break;
                }
            }

            app_wifi_ap_list_update(&s_scan_ap_list);
            s_ap_sel = ap_list_get_sel(&s_set_ap);

            if(s_ap_sel == -1) //ap lost
            {
                s_ap_sel = 0;
            }

            GUI_SetProperty(LIST_WIFI, "update_all", NULL);
            GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
        }
        else
        {
            app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
        }
    }
    else
    {
        app_wifi_ap_list_clean();
        s_ap_sel = -1;
        app_wifi_hide_gif();
        app_wifi_tip_hide();
        GUI_SetProperty(LIST_WIFI, "update_all", NULL);
        GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
        GUI_SetProperty("text_wifi_dev", "string", NULL);
        app_wifi_tip_show(STR_ID_NO_WIFI);
    }
    state_str = app_network_get_state_str(s_wifi_state);
    GUI_SetProperty("text_wifi_status", "string", state_str);
}

static void app_wifi_focus_change(const char *widget)
{
    if(widget == NULL)
        return;

    if(strcmp(widget, "connect to cancel") == 0)
    {
        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4");
        GUI_SetFocusWidget("button_wifi_cancel");
    }
    else if(strcmp(widget, "cancel to connect") == 0)
    {
        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4_l");
        GUI_SetFocusWidget("button_wifi_connect");
    }
    else if(strcmp(widget, "connect to psk") == 0 || strcmp(widget, "cancel to psk") == 0)
    {
        GUI_SetProperty("img_wifi_tip_opt","state","hide");
        GUI_SetProperty("img_wifi_tip_psk","state","show");
        GUI_SetFocusWidget("button_wifi_psk");
    }
    else if(strcmp(widget, "connect to essid") == 0 || strcmp(widget, "cancel to essid") == 0)
    {
        GUI_SetProperty("img_wifi_tip_opt","state","hide");
        GUI_SetProperty("img_wifi_tip_essid","state","show");
        GUI_SetProperty("text_wifi_set0", "backcolor", "[#ff00ff,#ff00ff,#ff00ff]");
        GUI_SetFocusWidget("button_wifi_essid");
    }
    else if(strcmp(widget, "connect to encrypt") == 0 || strcmp(widget, "cancel to encrypt") == 0)
    {
        GUI_SetProperty("img_wifi_tip_essid","state","hide");
        GUI_SetProperty("text_wifi_set0", "backcolor",TXT_WIFI_BK_COLOR);
        GUI_SetProperty("img_wifi_tip_opt","state","hide");
        GUI_SetProperty("img_wifi_tip_encryption","state","show");
        GUI_SetFocusWidget("cmb_wifi_encryption");
    }
    else if(strcmp(widget, "psk to connect") == 0)
    {
        GUI_SetProperty("img_wifi_tip_psk","state","hide");
        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4_l");
        GUI_SetProperty("img_wifi_tip_opt","state","show");
        GUI_SetProperty("button_wifi_cancel","string","Cancel");
        GUI_SetFocusWidget("button_wifi_connect");
    }
    else if(strcmp(widget, "psk to essid") == 0)
    {
        GUI_SetProperty("img_wifi_tip_psk","state","hide");
        GUI_SetProperty("img_wifi_tip_essid","state","show");
        GUI_SetProperty("text_wifi_set0", "backcolor", "[#ff00ff,#ff00ff,#ff00ff]");
        GUI_SetFocusWidget("button_wifi_essid");
    }
    else if(strcmp(widget, "essid to connect") == 0)
    {
        GUI_SetProperty("img_wifi_tip_essid","state","hide");
        GUI_SetProperty("text_wifi_set0", "backcolor",TXT_WIFI_BK_COLOR);
        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4_l");
        GUI_SetProperty("img_wifi_tip_opt","state","show");
        GUI_SetProperty("button_wifi_cancel","string","Cancel");
        GUI_SetFocusWidget("button_wifi_connect");
    }
    else if(strcmp(widget, "essid to psk") == 0)
    {
        GUI_SetProperty("img_wifi_tip_essid","state","hide");
        GUI_SetProperty("text_wifi_set0", "backcolor",TXT_WIFI_BK_COLOR);
        GUI_SetProperty("img_wifi_tip_psk","state","show");
        GUI_SetFocusWidget("button_wifi_psk");
    }
    else if(strcmp(widget, "encrypt to essid") == 0)
    {
        GUI_SetProperty("img_wifi_tip_essid","state","show");
        GUI_SetProperty("text_wifi_set0", "backcolor", "[#ff00ff,#ff00ff,#ff00ff]");
        GUI_SetProperty("img_wifi_tip_encryption","state","hide");
        GUI_SetFocusWidget("button_wifi_essid");
    }
    else if(strcmp(widget, "essid to encrypt") == 0)
    {
        GUI_SetProperty("img_wifi_tip_essid","state","hide");
        GUI_SetProperty("text_wifi_set0", "backcolor",TXT_WIFI_BK_COLOR);
        GUI_SetProperty("img_wifi_tip_encryption","state","show");
        GUI_SetProperty("text_wifi_set4", "state", "show");
        GUI_SetFocusWidget("cmb_wifi_encryption");
        //set sel;
    }
    else if(strcmp(widget, "encrypt to psk") == 0)
    {
        GUI_SetProperty("img_wifi_tip_encryption","state","hide");
        GUI_SetProperty("img_wifi_tip_psk","state","show");
        GUI_SetFocusWidget("button_wifi_psk");
    }
    else if(strcmp(widget, "psk to encrypt") == 0)
    {
        GUI_SetProperty("img_wifi_tip_encryption","state","show");
        GUI_SetProperty("img_wifi_tip_psk","state","hide");
        GUI_SetFocusWidget("cmb_wifi_encryption");
    }
    else if(strcmp(widget, "encrypt to connect") == 0)
    {
        GUI_SetProperty("img_wifi_tip_encryption","state","hide");
        GUI_SetProperty("img_wifi_tip_opt","img","s_bar_choice_blue4_l");
        GUI_SetProperty("img_wifi_tip_opt","state","show");
        GUI_SetProperty("button_wifi_cancel","string","Cancel");
        GUI_SetFocusWidget("button_wifi_connect");
    }
}

SIGNAL_HANDLER int app_wifi_button_essid_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_EXIT:
            case STBK_MENU:
                app_wifi_set_exit();
                break;
            case STBK_UP:
                app_wifi_focus_change("essid to connect");
                break;
            case STBK_DOWN:
                if(s_wifi_set_show == WIFI_SET_EDIT)
                {
                    if(s_wifi_encryption)
                        app_wifi_focus_change("essid to psk");
                    else
                        app_wifi_focus_change("essid to connect");
                }
                else if (s_wifi_set_show == WIFI_SET_ADD)
                {
                    app_wifi_focus_change("essid to encrypt");
                }

                break;
            case STBK_OK:
                {
                    static PopKeyboard keyboard;
                    memset(&keyboard, 0, sizeof(PopKeyboard));
                    keyboard.in_name    = s_set_ap.essid;
                    keyboard.max_num    = ESSID_MAX_SIZE;
                    keyboard.out_name   = NULL;
                    keyboard.change_cb  = NULL;
                    keyboard.release_cb = app_wifi_essid_keyboard_proc;
                    keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
                    keyboard.pos.x = 500;
                    multi_language_keyboard_create(&keyboard);
                }
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}
#if 1
SIGNAL_HANDLER int app_wifi_cmb_encryption_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_EXIT:
            case STBK_MENU:
                app_wifi_set_exit();
                ret = EVENT_TRANSFER_STOP;
                break;
            case STBK_UP:
                app_wifi_focus_change("encrypt to essid");
                ret = EVENT_TRANSFER_STOP;
                break;
            case STBK_DOWN:
                if(s_wifi_encryption == 0)
                    app_wifi_focus_change("encrypt to connect");
                else
                    app_wifi_focus_change("encrypt to psk");
                ret = EVENT_TRANSFER_STOP;
                break;
            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_wifi_cmb_encryption_change(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;

    GUI_GetProperty("cmb_wifi_encryption", "select", &sel);
    if(sel == 0) // encrypt off
    {
        GUI_SetProperty("button_wifi_set5","state", "disable");
        GUI_SetProperty("button_wifi_psk","state", "disable");
        s_wifi_encryption = 0;
        s_set_ap.encryption = 0;
    }
    else //on
    {
        GUI_SetProperty("button_wifi_set5","state", "enable");
        GUI_SetProperty("button_wifi_psk","state", "enable");
        s_wifi_encryption = 1;
        s_set_ap.encryption = 1;
    }

    return EVENT_TRANSFER_STOP;
}
#endif

SIGNAL_HANDLER int app_wifi_button_psk_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_EXIT:
            case STBK_MENU:
                app_wifi_set_exit();
                break;
            case STBK_UP:
                if(s_wifi_set_show == WIFI_SET_EDIT){
                    if(s_set_ap.hidden)
                        app_wifi_focus_change("psk to essid");
                    else
                        app_wifi_focus_change("psk to connect");
                }else if(s_wifi_set_show == WIFI_SET_ADD){
                    app_wifi_focus_change("psk to encrypt");
                }
                break;
            case STBK_DOWN:
                app_wifi_focus_change("psk to connect");
                break;
            case STBK_OK:
                {
                    static PopKeyboard keyboard;
                    memset(&keyboard, 0, sizeof(PopKeyboard));
                    keyboard.in_name    = s_set_ap.psk;
                    keyboard.max_num = sizeof(ubyte32)-1;
                    keyboard.out_name   = NULL;
                    keyboard.change_cb  = NULL;
                    keyboard.release_cb = app_wifi_full_keyboard_proc;
                    keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
                    keyboard.pos.x = 500;
                    multi_language_keyboard_create(&keyboard);
                }
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_button_connect_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
    GxMsgProperty_NetDevMode dev_mode;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_EXIT:
			case STBK_MENU:
                app_wifi_set_exit();
				break;
			case STBK_DOWN:
				if(s_wifi_connect)
					break;
				if(s_set_ap.hidden)
					app_wifi_focus_change("connect to essid");
				else
				{
					if(s_wifi_encryption)
						app_wifi_focus_change("connect to psk");
				}
				break;
			case STBK_UP:
				if(s_wifi_connect)
					break;
                if(s_wifi_set_show == WIFI_SET_EDIT){
                    if(s_wifi_encryption)
                        app_wifi_focus_change("connect to psk");
                    else if(s_set_ap.hidden)
                        app_wifi_focus_change("connect to essid");
                }else{
                    if(s_wifi_encryption)
                        app_wifi_focus_change("connect to psk");
                    else if(s_set_ap.hidden)
                        app_wifi_focus_change("connect to encrypt");
                }
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				app_wifi_focus_change("connect to cancel");
				break;
			case STBK_OK:
				if(s_wifi_connect == 1)
				{
					app_wifi_disconnect();
				}
				else
				{
					if(strlen(s_set_ap.essid) == 0)
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_OK;
						pop.str = STR_INPUT_ESSID;
						pop.mode = POP_MODE_UNBLOCK;
						pop.format = POP_FORMAT_DLG;
						popdlg_create(&pop);
						break;
					}
                    if(s_set_ap.encryption != 0 && strlen(s_set_ap.psk) < 8)
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_INPUT_PASSWD;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                        break;
                    }

                    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
                    strcpy(dev_mode.dev_name, s_wifi_dev[s_dev_sel]);
                    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
                    if(dev_mode.mode == 0)  //wifi disable
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_OPEN_WIFI;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                        break;
                    }
#ifdef ECOS_OS
					s_wifi_set_connecting = true;
#endif
                    if(s_wifi_set_show == WIFI_SET_EDIT)
                    {
                        app_wifi_save_ap(s_set_ap.hidden);
                        app_wifi_set_exit();
                    }
                    else if(s_wifi_set_show == WIFI_SET_ADD)
                    {
                        _wifi_scan_hidden_wifi();
                        app_wifi_set_exit();
                        app_wifi_tip_show(STR_ID_SCANNING_HIDDEN_AP);
                    }
                }
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_button_cancel_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
			case STBK_EXIT:
			case STBK_MENU:
				app_wifi_set_exit();
				break;
			case STBK_DOWN:
				if(s_wifi_connect)
					break;
				if(s_set_ap.hidden)
					app_wifi_focus_change("cancel to essid");
				else
				{
					if(s_wifi_encryption)
						app_wifi_focus_change("cancel to psk");
				}
				break;
			case STBK_UP:
				if(s_wifi_connect)
					break;
                if(s_wifi_set_show == WIFI_SET_EDIT){
                    if(s_wifi_encryption)
                        app_wifi_focus_change("cancel to psk");
                    else if(s_set_ap.hidden)
                        app_wifi_focus_change("cancel to essid");
                }else{
                    if(s_wifi_encryption)
                        app_wifi_focus_change("cancel to psk");
                    else if(s_set_ap.hidden)
                        app_wifi_focus_change("cancel to encrypt");
                }
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				app_wifi_focus_change("cancel to connect");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_wifi_list_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_list_list_change(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    GUI_GetProperty(LIST_WIFI, "select", &sel);
    s_ap_sel = sel;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_list_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
    int32_t sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_MENU:
			case STBK_EXIT:
                if(s_wifi_tip_flag)
                    app_wifi_tip_hide();
                else
                    GUI_EndDialog(WND_WIFI);
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_DOWN:
                if(s_wifi_tip_flag)
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
				break;
			case STBK_UP:
                if(s_wifi_tip_flag)
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
				break;

			case STBK_PAGE_DOWN:
                if(s_wifi_tip_flag)
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                if((s_wifi_dev_total>0)&&(s_ap_total>0))
                {
                    GUI_GetProperty(LIST_WIFI, "select", &sel);
                    if(sel == s_ap_total-1)
                    {
                        sel = 0;
                        GUI_SetProperty(LIST_WIFI, "select", &sel);
                        GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                        ret =   EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret =  EVENT_TRANSFER_KEEPON;
                    }
                }
                break;
            case STBK_PAGE_UP:
                if(s_wifi_tip_flag)
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                if((s_wifi_dev_total>0)&&(s_ap_total>0))
                {
                    GUI_GetProperty(LIST_WIFI, "select", &sel);
                    if(sel == 0)
                    {
                        sel = s_ap_total-1;
                        GUI_SetProperty(LIST_WIFI, "select", &sel);
                        GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                        ret =   EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret =  EVENT_TRANSFER_KEEPON;
                    }
                }
                break;
			case STBK_GREEN:
                if(s_wifi_dev_total > 0)
                    app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
				break;
            case STBK_RED: // Add wifi
                if(false == s_wifi_tip_flag && false == s_wifi_ap_updating)
                    app_wifi_set_show(WIFI_SET_ADD);
                break;
			case STBK_OK:
                if(s_wifi_dev_total > 0)
                    app_wifi_set_show(WIFI_SET_EDIT);
				ret = EVENT_TRANSFER_STOP;
				break;
			default:
				break;
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_wifi_list_get_total(const char* widgetname, void *usrdata)
{
	return s_ap_total;
}

SIGNAL_HANDLER int app_wifi_list_get_data(const char* widgetname, void *usrdata)
{
	ListItemPara* item = NULL;
	static char buf[8];
	WifiApPara *p_ap = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	p_ap = ap_list_get(item->sel);
	if(p_ap == NULL)
		return GXCORE_ERROR;

	//col-0
	item->x_offset = 5;
	item->image = NULL;
	memset(buf,  0, 8);
	sprintf(buf, "%d. ", item->sel+1);
	item->string = buf;

	//col-1
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = p_ap->essid;

	//col-2
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	if(p_ap->is_link == 1)
        item->image = "s_choice";
    else
        item->image = NULL;

    //col-3
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	if(p_ap->encryption != 0)
		item->image = "s_lock";
	else
		item->image = NULL;

	//col-4
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	item->image = s_img_strength[p_ap->quality_level];

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_create(const char* widgetname, void *usrdata)
{
    APP_FREE(s_scan_ap_list.ap_info);
    s_scan_ap_list.ap_info = GxCore_Calloc(MAX_AP_NUM, sizeof(ApInfo));
    if(NULL == s_scan_ap_list.ap_info)
    {
        app_log_error("calloc error\n");
        return EVENT_TRANSFER_STOP;
    }

    app_wifi_load_gif();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_destroy(const char* widgetname, void *usrdata)
{
	s_wifi_linking= FALSE;
	s_ap_total = 0;
	s_ap_sel = 0;

    if(s_wifi_init_timer != NULL)
    {
        remove_timer(s_wifi_init_timer);
        s_wifi_init_timer = NULL;
    }

    app_wifi_hide_gif();
    app_wifi_free_gif();

	if(s_wifi_dev)
	{
		GxCore_Free(s_wifi_dev);
		s_wifi_dev = NULL;
	}

	ap_list_destroy();
    APP_FREE(s_scan_ap_list.ap_info);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wifi_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if(s_wifi_dev_total ==0 && event->key.sym != STBK_EXIT && event->key.sym != STBK_MENU && event->key.sym != VK_BOOK_TRIGGER)
		{
			return EVENT_TRANSFER_STOP;
		}
		switch(event->key.sym)
		{
			case VK_BOOK_TRIGGER:
				s_wifi_set_show = WIFI_SET_HIDE;
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_RIGHT:
				if(s_wifi_dev_total == 1 || s_wifi_dev_total == 0)
					return EVENT_TRANSFER_STOP;
				if(s_dev_sel < s_wifi_dev_total-1)
					s_dev_sel++;
				else
					s_dev_sel = 0;

                app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
				break;
			case STBK_LEFT:
				if(s_wifi_dev_total == 1 || s_wifi_dev_total == 0)
					return EVENT_TRANSFER_STOP;
				if(s_dev_sel >0)
					s_dev_sel--;
				else
					s_dev_sel = s_wifi_dev_total-1;

                app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
				break;

            default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

//plz free wifi_dev if ret > 0 with GxCore_Free()
uint32_t app_get_wifi_list(ubyte64 **wifi_dev)
{
    uint32_t i = 0;
    uint32_t wifi_num = 0;
    GxMsgProperty_NetDevList dev_list;
    GxMsgProperty_NetDevType type;
    ubyte64 *temp = NULL;

    if(wifi_dev == NULL)
        return 0;

    memset(&dev_list, 0, sizeof(GxMsgProperty_NetDevList));
    app_send_msg_exec(GXMSG_NETWORK_DEV_LIST, &dev_list);

    for(i = 0; i < dev_list.dev_num; i++)
    {
        memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
        strcpy(type.dev_name, dev_list.dev_name[i]);
        app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
        if(type.dev_type == IF_TYPE_WIFI)
        {
            if((temp = GxCore_Realloc(*wifi_dev, sizeof(ubyte64) * (wifi_num + 1))) != NULL)
            {
                *wifi_dev = temp;
                strcpy((*wifi_dev)[wifi_num], dev_list.dev_name[i]);
                app_log_debug("~~~~~~[%s]%d, wifi: %s!!!!!\n", __func__, __LINE__, (*wifi_dev)[wifi_num]);
                wifi_num++;
            }
        }
    }
    return wifi_num;
}

static uint32_t app_wifi_dev_list_update(ubyte64 **wifi_dev)
{
    ubyte64 *wifi_list = NULL;
    uint32_t wifi_num = 0;
    uint32_t i = 0;

    if(wifi_dev == NULL)
	{
        return 0;
	}

    wifi_num = app_get_wifi_list(&wifi_list);
    if((wifi_num > 0) && (wifi_list != NULL))
    {
        if(s_wifi_menu_mode == WIFI_MENU_ALL) //all mode
        {
            if((*wifi_dev = (ubyte64*)GxCore_Calloc(sizeof(ubyte64), wifi_num)) != NULL)
            {
                for(i = 0; i < wifi_num; i++)
                {
                    strcpy((*wifi_dev)[i], wifi_list[i]);
                }
            }
        }
        else //single mode
        {
            for(i = 0; i < wifi_num; i++)
            {
                if(strcmp(wifi_list[i], s_cur_dev) == 0)
                {
                    wifi_num = 1;
                    if((*wifi_dev = (ubyte64*)GxCore_Calloc(sizeof(ubyte64), wifi_num)) != NULL)
                    {
                        strcpy(**wifi_dev, s_cur_dev);
                    }
                    break;
                }
            }
        }

        GxCore_Free(wifi_list);
        wifi_list = NULL;
    }
    return wifi_num;
}

void _wifi_ap_scan_msg_proc(GxMsgProperty_ApList *ap_list)
{
    const char *wnd = GUI_GetFocusWindow();

    if(ap_list == NULL)
        return;

    if((s_wifi_dev == NULL) || (strcmp(s_wifi_dev[s_dev_sel], ap_list->wifi_dev) != 0))
        return;

    app_log_info("~~~~~~[%s]%d, %s ap scan over, %d ap!!!!!\n", __func__, __LINE__, ap_list->wifi_dev, ap_list->ap_num);

    strlcpy(s_scan_ap_list.wifi_dev, ap_list->wifi_dev, sizeof(ap_list->wifi_dev));
    s_scan_ap_list.ap_num = ap_list->ap_num > MAX_AP_NUM?MAX_AP_NUM:ap_list->ap_num;
    memcpy(s_scan_ap_list.ap_info, ap_list->ap_info, s_scan_ap_list.ap_num * sizeof(ApInfo));

    if((wnd != NULL) /*&& (strcmp(wnd, WND_WIFI) == 0) */&& (s_wifi_set_show == WIFI_SET_HIDE))
    {
        s_ap_sel = 0;
        app_wifi_ap_list_update(&s_scan_ap_list);
        if(s_ap_total == 0)
        {
            app_wifi_tip_show(STR_ID_NO_AP);
            s_ap_sel = -1;
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(1000);
        }
        else
        {
            app_wifi_tip_hide();
            GUI_SetProperty(LIST_WIFI, "update_all", NULL);
            GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
            app_update_pop_dlg(WND_POP_TIP);
            app_update_pop_dlg(WND_POP_BOOK);
            GUI_SetInterface("flush", NULL);
        }
    }
}

static void _wifi_ap_scan_hidden_msg_proc(GxMsgProperty_ApList *ap_list)
{
    int i = 0;
    unsigned short encryption = 0;

    if(NULL == ap_list || NULL == s_wifi_dev || strcmp(s_wifi_dev[s_dev_sel], ap_list->wifi_dev) != 0)
        return;

    app_wifi_tip_hide();
    for(i = 0; i < ap_list->ap_num; i++)
    {
        encryption = ap_list->ap_info[i].encryption != 0?1:0;
        if(strlen(s_set_ap.essid) && 0 == strcmp(ap_list->ap_info[i].ap_essid, s_set_ap.essid) && s_set_ap.encryption == encryption)
        {
            if(IF_STATE_CONNECTED == s_wifi_state)
            {
                app_wifi_disconnect();
                s_ap_sel = 0;
                app_wifi_ap_list_update(&s_scan_ap_list);
                if(WIFI_SET_HIDE == s_wifi_set_show)
                {
                    GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                    GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
                    app_update_pop_dlg(WND_POP_TIP);
                    app_update_pop_dlg(WND_POP_BOOK);
                }
            }
            strcpy(s_set_ap.mac, ap_list->ap_info[i].ap_mac);
            app_wifi_save_ap(s_set_ap.hidden);
            return;
        }
    }

    if(WIFI_SET_HIDE == s_wifi_set_show)
        app_wifi_tip_show(STR_ID_NO_AP);
    app_log_info("~~~~~~[%s]%d, %s hidden ap scan over, %d ap!!!!!\n", __func__, __LINE__, ap_list->wifi_dev, ap_list->ap_num);
}

void _wifi_state_msg_proc(GxMsgProperty_NetDevState *wifi_state)
{
    const char *wnd = GUI_GetFocusWindow();
    char *state_str = NULL;

    if(wifi_state == NULL)
        return;

    if((s_wifi_dev == NULL) || (strcmp(s_wifi_dev[s_dev_sel], wifi_state->dev_name) != 0))
        return;

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_WIFI))
    {
        if(((wifi_state->dev_state == IF_STATE_CONNECTING) || (wifi_state->dev_state == IF_STATE_START))&& (s_wifi_set_show == WIFI_SET_HIDE))
        {
            popdlg_destroy();
            app_wifi_show_gif();
#ifdef ECOS_OS
			s_wifi_set_connecting = true;
#endif
        }
        else
        {
			if(s_wifi_tip_flag == true)
			{
				s_wifi_ap_updating = false;
				app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
			}
            app_wifi_hide_gif();
        }

        if(wifi_state->dev_state == IF_STATE_CONNECTED)
        {
            if(s_wifi_set_show)
            {
                popdlg_destroy();
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                    GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
                GUI_SetInterface("flush", NULL);
                app_wifi_set_hide();
            }

#ifdef ECOS_OS
            ubyte32 bssid={0};
            app_get_wifi_bssid(wifi_state->dev_name,bssid);
            if(strlen(s_cur_ap.ap_mac) && strcmp(s_cur_ap.ap_mac, bssid) != 0)
            {
                if(s_wifi_tip_flag)
                    app_wifi_tip_hide();

                app_log_debug("%s::%s\n",s_cur_ap.ap_mac,bssid);
                app_wifi_linkdown(wifi_state->dev_name);
                s_wifi_state = wifi_state->dev_state;
                GUI_SetProperty("text_wifi_status", "string", STR_ID_ERR_PASSWD);
                return;
            }
#endif

            if(false == s_wifi_ap_updating)
            {
                int i = 0;
                for(i = 0; i < s_scan_ap_list.ap_num; i++)
                {
                    if(strcmp(s_set_ap.mac, s_scan_ap_list.ap_info[i].ap_mac) == 0)
                    {
                        strcpy(s_scan_ap_list.ap_info[i].ap_psk, s_set_ap.psk);
                        if(s_set_ap.hidden)
                        {
                            strcpy(s_scan_ap_list.ap_info[i].ap_essid, s_set_ap.essid);
                        }
                        break;
                    }
                }
                s_ap_sel = 0;
                app_wifi_ap_list_update(&s_scan_ap_list);
                if(s_wifi_tip_flag)
                {
                    app_wifi_tip_hide();
                }
                GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
            }
        }
        else if(wifi_state->dev_state == IF_STATE_IDLE)
        {
            memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
            if(s_wifi_state == IF_STATE_CONNECTED) //stop
            {
                app_wifi_ap_list_update(&s_scan_ap_list);
                if(s_wifi_tip_flag)
                    app_wifi_tip_hide();
                GUI_EndDialog("after wnd_wifi");
                if(s_wifi_set_show != WIFI_SET_HIDE)
                  app_wifi_set_hide();
                GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                GUI_SetInterface("flush", NULL);
            }
            else if((s_wifi_state == IF_STATE_CONNECTING) || (s_wifi_state == IF_STATE_START))//fail
            {
            }
        }
        else if(wifi_state->dev_state == IF_STATE_REJ)
        {
            popdlg_destroy();
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                GUI_EndDialog(WND_KEYBOARD_LANGUAGE);

            app_wifi_set_exit();
            app_wifi_hide_gif();
            if(s_wifi_tip_flag)
                app_wifi_tip_hide();

            GUI_SetProperty("text_wifi_status", "string", STR_ID_ERR_PASSWD);
            app_wifi_disconnect();
        }
        else if(wifi_state->dev_state == IF_STATE_DISCONNECTED && s_wifi_state != wifi_state->dev_state)
        {
            if((wnd != NULL) && (0 == strcmp(wnd, WND_WIFI)) && (s_wifi_set_show == WIFI_SET_HIDE))
            {
                app_wifi_ap_list_update(&s_scan_ap_list);
                GUI_SetProperty(LIST_WIFI, "update_all", NULL);
                app_wifi_hide_gif();
                if(s_wifi_tip_flag) app_wifi_tip_hide();
                memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
            }
        }
    }
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);

    s_wifi_state = wifi_state->dev_state;
    if(!s_wifi_set_show)
    {
        state_str = app_network_get_state_str(s_wifi_state);
        GUI_SetProperty("text_wifi_status", "string", state_str);
    }
}

static void _wifi_plug_in_proc(ubyte64 *wifi_dev)
{
    ubyte64 cur_dev = {0};
    uint32_t dev_num = 0;
    uint32_t i = 0;
    const char *wnd = GUI_GetFocusWindow();

    if(wifi_dev == NULL)
        return;

    if(s_wifi_dev != NULL)
    {
        strcpy(cur_dev, s_wifi_dev[s_dev_sel]);
        GxCore_Free(s_wifi_dev);
        s_wifi_dev = NULL;
    }

    dev_num = app_wifi_dev_list_update(&s_wifi_dev);
    if((dev_num > 0) && (s_wifi_dev != NULL))
    {
        s_dev_sel = 0;
        for(i = 0; i < dev_num; i++)
        {
            if(strcmp(cur_dev, s_wifi_dev[i]) == 0)
            {
                s_dev_sel = i;
                break;
            }
        }

        if((wnd != NULL) && (strcmp(wnd, WND_WIFI) == 0) && (s_wifi_set_show == WIFI_SET_HIDE))
        {
            if(dev_num > 1)
            {
                GUI_SetProperty("img_wifi_l","state","show");
                GUI_SetProperty("img_wifi_r","state","show");
            }
            else
            {
                GUI_SetProperty("img_wifi_l","state","hide");
                GUI_SetProperty("img_wifi_r","state","hide");
            }

            if(s_wifi_dev_total == 0)
            {
                app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
            }
        }

        s_wifi_dev_total = dev_num;
    }
    else
    {
        s_wifi_dev_total = 0;
        if((wnd != NULL) && (strcmp(wnd, WND_WIFI) == 0) && (s_wifi_set_show == WIFI_SET_HIDE))
        {
            app_wifi_ap_list_clean();
            app_wifi_hide_gif();
            app_wifi_tip_hide();
            GUI_SetProperty(LIST_WIFI, "update_all", NULL);
            GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
            GUI_SetProperty("text_wifi_dev", "string", NULL);
            app_wifi_tip_show(STR_ID_NO_WIFI);
        }
    }
}

static void _wifi_plug_out_proc(ubyte64 *wifi_dev)
{
    ubyte64 cur_dev = {0};
    uint32_t i = 0;
    const char *wnd = GUI_GetFocusWindow();

    if(wifi_dev == NULL)
        return;

    if(s_wifi_dev != NULL)
    {
        strcpy(cur_dev, s_wifi_dev[s_dev_sel]);
        GxCore_Free(s_wifi_dev);
        s_wifi_dev = NULL;
    }

    if(s_wifi_set_show)
    {
        GUI_SetProperty(WND_WIFI, "update", NULL);
        app_wifi_set_hide();
    }

    s_wifi_dev_total = app_wifi_dev_list_update(&s_wifi_dev);
    if((s_wifi_dev_total > 0) && (s_wifi_dev != NULL))
    {
        s_dev_sel = 0;
        for(i = 0; i < s_wifi_dev_total; i++)
        {
            if(strcmp(cur_dev, s_wifi_dev[i]) == 0)
            {
                s_dev_sel = i;
                break;
            }
        }

        if((wnd != NULL) && (strcmp(wnd, WND_WIFI) == 0) && (s_wifi_set_show == WIFI_SET_HIDE))
        {
            if(s_wifi_dev_total > 1)
            {
                GUI_SetProperty("img_wifi_l","state","show");
                GUI_SetProperty("img_wifi_r","state","show");
            }
            else
            {
                GUI_SetProperty("img_wifi_l","state","hide");
                GUI_SetProperty("img_wifi_r","state","hide");
            }

            if(i > s_wifi_dev_total)
            {
                app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
            }
        }
    }
    else
    {
        if((wnd != NULL) && (strcmp(wnd, WND_WIFI) == 0) && (s_wifi_set_show == WIFI_SET_HIDE))
        {
            app_wifi_ap_list_clean();
            s_ap_sel = -1;
            app_wifi_hide_gif();
            app_wifi_tip_hide();
            GUI_SetProperty(LIST_WIFI, "update_all", NULL);
            GUI_SetProperty(LIST_WIFI, "select", &s_ap_sel);
            GUI_SetProperty("text_wifi_dev", "string", NULL);
            app_wifi_tip_show(STR_ID_NO_WIFI);
        }
    }
}

void app_wifi_msg_set_ap_false(void)
{
	s_wifi_ap_updating = false;
}

void app_wifi_msg_proc(GxMessage *msg)
{
    GxMsgProperty_NetDevType type;

    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_WIFI_SCAN_AP_OVER:
        {
            GxMsgProperty_ApList *ap_list = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_ApList);
            if(ap_list != NULL)
            {
                _wifi_ap_scan_msg_proc(ap_list);
            }
            break;
        }

        case GXMSG_WIFI_SCAN_HIDDEN_AP_OVER:
        {
            GxMsgProperty_ApList *ap_list = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_ApList);
            if(ap_list != NULL)
            {
                _wifi_ap_scan_hidden_msg_proc(ap_list);
            }
            break;
        }

        case GXMSG_NETWORK_STATE_REPORT:
        {
            GxMsgProperty_NetDevState *wifi_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            if(wifi_state != NULL)
            {
                memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                strcpy(type.dev_name, wifi_state->dev_name);
                app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
                if(type.dev_type == IF_TYPE_WIFI)
                {
                    _wifi_state_msg_proc(wifi_state);
                }
            }
            break;
        }

        case GXMSG_NETWORK_PLUG_IN:
        {
			ubyte64 *wifi_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            if(wifi_dev != NULL)
            {
                //app_log_info("~~~~~~[%s]%d, wifi %s plug in!!!!!\n", __func__, __LINE__, *wifi_dev);
                memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                strcpy(type.dev_name, *wifi_dev);
                app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
                if(type.dev_type == IF_TYPE_WIFI)
                {
                    _wifi_plug_in_proc(wifi_dev);
                }
            }
            break;
        }

        case GXMSG_NETWORK_PLUG_OUT:
        {
            ubyte64 *wifi_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            if(wifi_dev != NULL)
            {
                //app_log_info("~~~~~~[%s]%d, wifi %s plug out!!!!!\n", __func__, __LINE__, *wifi_dev);
                memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                strcpy(type.dev_name, *wifi_dev);
                app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
                if(type.dev_type == IF_TYPE_WIFI)
                {
                    if ( (app_network_wifi_get_all_dev_plug_count() >= 1) &&
                         (strncmp(s_cur_dev,*wifi_dev,NET_IF_DEV_WIFI_NLEN) != 0) )
                    {
                        break;
                    }
#ifdef ECOS_OS
                    s_wifi_set_connecting = false;
#endif
                    if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
                    {
                        app_pop_list_end_dialog();
                    }
                    popdlg_destroy();
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                        GUI_EndDialog(WND_KEYBOARD_LANGUAGE);

                    _wifi_plug_out_proc(wifi_dev);
                    GUI_EndDialog(WND_WIFI);
                }
            }
            break;
        }

        default:
            break;
    }
}

status_t app_create_wifi_menu(ubyte64 *wifi_dev)
{
    if(wifi_dev == NULL)
    {
        return GXCORE_ERROR;
    }

    s_dev_sel = 0;
    strcpy(s_cur_dev, *wifi_dev);

    GUI_CreateDialog(WND_WIFI);

    if(strcmp(*wifi_dev, "WIFI-ALL") == 0) //all mode
        s_wifi_menu_mode = WIFI_MENU_ALL;
    else
        s_wifi_menu_mode = WIFI_MENU_SINGLE;

    s_wifi_dev_total = 0;
    if(s_wifi_dev)
	{
		GxCore_Free(s_wifi_dev);
		s_wifi_dev = NULL;
	}

    s_wifi_dev_total = app_wifi_dev_list_update(&s_wifi_dev);
    if((s_wifi_dev_total > 0) && (s_wifi_dev != NULL))
    {
        if(s_wifi_dev_total > 1)
		{
			GUI_SetProperty("img_wifi_l","state","show");
			GUI_SetProperty("img_wifi_r","state","show");
		}
		else
		{
			GUI_SetProperty("img_wifi_l","state","hide");
			GUI_SetProperty("img_wifi_r","state","hide");
		}
        app_wifi_dev_update(s_wifi_dev[s_dev_sel]);
    }
    else
        app_wifi_tip_show(STR_ID_NO_WIFI);

    return GXCORE_SUCCESS;
}
#endif //NETWORK_SUPPORT && WIFI_SUPPORT
