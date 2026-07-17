#include "app.h"
#if NET_SEARCH_SUPPORT
#include "module/app_frontend_board_config.h"
#include "app_common_search_setting.h"
#include "app_keyboard_language.h"
#include "module/app_frontend.h"
#include "app_net_search_list.h"
#include "app_default_params.h"
#include "app_net_search_api.h"
#include "app_wnd_file_list.h"
#include "app_volume_value.h"
#include "app_wnd_search.h"
#include "module/app_log.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_module.h"
#include "app_book.h"
#include "app_epg.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#endif


//*************************************************************************
#define BUFFER_LENGTH_SAT	(MAX_SAT_NAME+10)
#define MAX_URL_LEN			"20"
#define NET_SEARCH_CONTENT  "[Server search,Stream search]"
#define YES_NO_CONTENT		"[No,Yes]"
#define APP_MAX_NET_URL_LEN (1024)
#define TIMEOUT_PAT 5000
#define TIMEOUT_PMT 7000
#define TIMEOUT_SDT 7000
#define TIMEOUT_NIT 10000

typedef enum
{
    ITEM_NET_SERVER_NAME,
    ITEM_NET_SERVER_URL,
    ITEM_NET_STREAM_URL,
#if DVB_SERVER_SEARCH_SUPPORT
    ITEM_NET_USER_NAME,
    ITEM_NET_PASSWORD,
#endif
    ITEM_NET_FTA_ONLY,
    ITEM_NET_SEARCH_START,
    ITEM_NET_SEARCH_TOTAL,
}NetSearchItem;

typedef enum
{
	SEARCH_SEL_SERVER = 0,
	SEARCH_SEL_STREAM,
}NetSearchMode;

typedef enum
{
	FTA_NO = 0,
	FTA_YES,
}FtaOnly;

typedef enum
{
	SERVER_TYPE = 0,
	STREAM_TYPE,
	DEF_TYPE,
}ServerListType;

typedef enum
{
	MODE_DEL = 0,
	MODE_ADD,
	MODE_DEF,
}ServerEditMode;

typedef struct
{
    uint32_t	cur_net_server_sel;
    uint32_t	cur_net_server_id;
    uint32_t	net_server_total;
	uint32_t	*net_server_id_arry;
    uint32_t	cur_net_stream_sel;
    uint32_t	cur_net_stream_id;
	uint32_t	*cur_stream_id_arry;
    uint32_t	cur_server_stream_total;
	uint32_t    cur_item;
	char		*net_server_name_content;
	char		cur_net_server_name[MAX_SAT_NAME];
    char		*cur_net_server_url;
#if DVB_SERVER_SEARCH_SUPPORT
    char		cur_server_user_name[USER_NAME_LEN];
    char		cur_server_password[PASSWORD_LEN];
#endif
    char		*cur_net_stream_url;
	char		*net_search_start_content;
	AppIdOps*	serverIdOps;
	AppIdOps*	streamIdOps;
	FtaOnly		fta_mode;
	NetSearchMode	search_mode;
	ServerListType list_type;
	ServerEditMode ctrl_mode;
	CSLOps      Listpara;
}ServerSearchUICrtl;

static GxMsgProperty_ScanNetStart s_NetSearch;
//*********************function***************************************//
static status_t app_net_stream_list_del(void);
static uint32_t app_get_net_server_num(void);
static int app_net_search_name_cmb_change(int sel);
static int app_net_search_name_cmb_keypress(void);
static void app_net_search_setting_data_init(void);
static int app_net_search_setting_red_key(void);
static int app_net_search_setting_blue_key(void);
static int app_net_search_setting_green_key(void);
static int app_net_search_setting_yellow_key(void);
static int app_net_search_setting_item_change(int sel);
static uint32_t app_get_current_server_stream_num(void);
static status_t app_net_sat_name_build(char **content_buffer);
static status_t app_delete_net_server(uint32_t *delete_arry, uint32_t delete_num);
static status_t app_delete_net_stream(uint32_t *delete_arry, uint32_t delete_num);
static status_t app_delete_net_stream_url_by_stream_id(uint32_t *delete_arry, uint32_t delete_num);
static status_t app_delete_net_server_url_by_server_id(uint32_t *delete_id_arry, uint32_t delete_num);
#if DVB_SERVER_SEARCH_SUPPORT
static bool app_get_server_user_name(char *user_name);
static bool app_get_server_password(char *password);
#endif
extern void app_search_set_search_type(SearchType type);
//********************variable********************************//
static CSTOpt thiz_Net_SearchOpt = {0};
static CSTItem thiz_NetSearchItem[ITEM_NET_SEARCH_TOTAL];

static ServerSearchUICrtl s_search_ctrl = {
    .cur_net_server_sel = 0,
    .cur_net_server_id = 0,
    .net_server_total = 0,
	.net_server_id_arry = NULL,
    .net_server_name_content = NULL,
	.cur_net_server_name = {0},
    .cur_net_server_url = NULL,
#if DVB_SERVER_SEARCH_SUPPORT
	.cur_server_user_name = {0},
	.cur_server_password = {0},
#endif
    .cur_net_stream_sel = 0,
    .cur_net_stream_id = 0,
	.cur_stream_id_arry = NULL,
    .cur_net_stream_url = NULL,
    .cur_server_stream_total = 0,
	.fta_mode = FTA_NO,
	.search_mode = SEARCH_SEL_STREAM,
	.cur_item = 0,
	.serverIdOps = NULL,
	.streamIdOps = NULL,
	.net_search_start_content = NET_SEARCH_CONTENT,
	.Listpara = {0},
	.list_type = DEF_TYPE,
	.ctrl_mode = MODE_DEF,
};


//****************************************************************//

static uint32_t app_get_server_index_by_sat_id(uint32_t server_id)
{
	GxMsgProperty_NodeByIdGet node_sat = {0};

	node_sat.node_type = NODE_SAT;
	node_sat.id = server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);
    if(GXBUS_PM_SAT_NET != node_sat.sat_data.type)
		return 0;
	return node_sat.sat_data.sat_net.index;
}

static uint32_t _get_stream_index_by_stream_id(uint32_t stream_id)
{
	GxMsgProperty_NodeByIdGet node_tp = {0};

	node_tp.node_type = NODE_TP;
	node_tp.id = stream_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);
	return node_tp.tp_data.tp_net.index;
}

static uint32_t app_get_net_server_num(void)
{
    uint32_t sat_count = 0;
    uint32_t i;
	GxMsgProperty_NodeNumGet node_num = {0};

	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	if(node_num.node_num == 0)
		return 0;

	GxMsgProperty_NodeByPosGet node_sat = {0};
	for(i = 0; i < node_num.node_num; i++)
	{
		node_sat.node_type = NODE_SAT;
		node_sat.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
		if(GXBUS_PM_SAT_NET != node_sat.sat_data.type)
			continue;
		sat_count++;
	}

    return sat_count;
}

static uint32_t app_get_current_server_stream_num(void)
{
    GxMsgProperty_NodeNumGet node_num = {0};

	node_num.node_type = NODE_TP;
	node_num.sat_id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	return node_num.node_num;
}

static status_t app_net_sat_name_build(char **content_buffer)
{
    char *sat_default_name = "[NONE]";
    char buffer[BUFFER_LENGTH_SAT] = {0};
    uint32_t sat_count = 0;
    uint32_t i = 0;
	GxMsgProperty_NodeByPosGet node_sat = {0};
    GxMsgProperty_NodeNumGet node_num = {0};

    s_search_ctrl.net_server_total = app_get_net_server_num();
    if(s_search_ctrl.net_server_total == 0)
    {
        app_log_debug("get net satellite num is %d\n",s_search_ctrl.net_server_total);
        *content_buffer = GxCore_Calloc(BUFFER_LENGTH_SAT, sizeof(char));
        if(*content_buffer == NULL)
        {
            app_log_error("[%s]:[%d] GxCore_Calloc error\n",__func__, __LINE__);
            return GXCORE_ERROR;
        }
        strcpy(*content_buffer, sat_default_name);
        return GXCORE_SUCCESS;
    }
    else
    {
		APP_FREE(*content_buffer);
        *content_buffer = GxCore_Calloc(BUFFER_LENGTH_SAT * s_search_ctrl.net_server_total, sizeof(char));
		APP_FREE(s_search_ctrl.net_server_id_arry);
		s_search_ctrl.net_server_id_arry = GxCore_Calloc(s_search_ctrl.net_server_total, sizeof(uint32_t));
        if(*content_buffer == NULL || s_search_ctrl.net_server_id_arry == NULL)
        {
            app_log_error("[%s]:[%d] GxCore_Calloc error\n",__func__, __LINE__);
			APP_FREE(*content_buffer);
			APP_FREE(s_search_ctrl.net_server_id_arry);
            return GXCORE_ERROR;
        }
    }
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    strcat(*content_buffer, "[");
	for(i = 0; i < node_num.node_num; i++)
	{
		node_sat.node_type = NODE_SAT;
		node_sat.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(GXBUS_PM_SAT_NET != node_sat.sat_data.type)
			continue;
		s_search_ctrl.net_server_id_arry[sat_count++] = node_sat.sat_data.id;
		memset(buffer, 0, BUFFER_LENGTH_SAT);
        snprintf(buffer,BUFFER_LENGTH_SAT,"(%d/%d) %s,", sat_count, s_search_ctrl.net_server_total, (char*)node_sat.sat_data.sat_net.server_name);
		strcat(*content_buffer, buffer);
	}
    strcat(*content_buffer, "]");
    return GXCORE_SUCCESS;
}

static status_t app_get_cur_net_server_name(char *server_name)
{
	GxMsgProperty_NodeByIdGet node_sat = {0};

	if (server_name == NULL)
		return GXCORE_ERROR;
	else
		memset(server_name, 0, MAX_SAT_NAME);
	s_search_ctrl.net_server_total = app_get_net_server_num();
	if(s_search_ctrl.net_server_total == 0)
	{
		return GXCORE_ERROR;
	}
	else
	{
		node_sat.node_type = NODE_SAT;
		node_sat.id = s_search_ctrl.cur_net_server_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);
		if(strlen((char*)node_sat.sat_data.sat_net.server_name) != 0)
		{
			strcpy((char*)server_name, (char*)node_sat.sat_data.sat_net.server_name);
			return GXCORE_SUCCESS;
		}
	}
	return GXCORE_ERROR;
}

static status_t app_get_current_net_server_url(char **server_url)
{
	char *server_default_url = STR_ID_NONE;
    uint32_t server_index = 0;

	if(*server_url != NULL)
	{
		memset(*server_url, 0, APP_MAX_NET_URL_LEN);
	}
	else
	{
        *server_url = GxCore_Calloc(APP_MAX_NET_URL_LEN, sizeof(char));
		if(*server_url == NULL)
		{
            app_log_error("[%s]:[%d] GxCore_Calloc error\n",__func__, __LINE__);
            return GXCORE_ERROR;
		}
	}
    s_search_ctrl.net_server_total = app_get_net_server_num();
    if(s_search_ctrl.net_server_total == 0)
    {
        strcpy(*server_url, server_default_url);
        return GXCORE_SUCCESS;
    }
	server_index = app_get_server_index_by_sat_id(s_search_ctrl.cur_net_server_id);
	if(server_index == 0)
	{
        strcpy(*server_url, server_default_url);
		return GXCORE_SUCCESS;
	}
	GxBus_PmGetNetServerUrlByIndex(&server_index, server_url, 1);
	if(strlen(*server_url) == 0)
	{
        strcpy(*server_url, server_default_url);
	}
    return GXCORE_SUCCESS;
}

static status_t app_get_current_stream_url(char **stream_url)
{
	char *stream_default_url = STR_ID_NONE;
	uint32_t stream_index = 0;

	if(*stream_url != NULL)
	{
		memset(*stream_url, 0, APP_MAX_NET_URL_LEN);
	}
	else
	{
		*stream_url = GxCore_Calloc(APP_MAX_NET_URL_LEN, sizeof(char));
		if(*stream_url == NULL)
		{
            app_log_error("[%s]:[%d] GxCore_Calloc error\n",__func__, __LINE__);
			return GXCORE_ERROR;
		}
	}
	stream_index = _get_stream_index_by_stream_id(s_search_ctrl.cur_net_stream_id);
	if(stream_index == 0)
	{
        strcpy(*stream_url, stream_default_url);
        return GXCORE_SUCCESS;
	}
	GxBus_PmGetNetStreamUrlByIndex(&stream_index, stream_url, 1);
	if(strlen(*stream_url) == 0)
	{
        strcpy(*stream_url, stream_default_url);
	}
    return GXCORE_SUCCESS;
}

static status_t app_set_cur_stream_sel(int sel)
{
	if(sel >= s_search_ctrl.cur_server_stream_total && s_search_ctrl.cur_stream_id_arry != NULL)
		return GXCORE_ERROR;
	s_search_ctrl.cur_net_stream_sel = sel;
	s_search_ctrl.cur_net_stream_id = s_search_ctrl.cur_stream_id_arry[s_search_ctrl.cur_net_stream_sel];

	return GXCORE_SUCCESS;
}

static status_t app_build_stream_id_arry(void)
{
	static uint32_t max_arry_num = 0;
	GxMsgProperty_NodeByPosGet node_tp = {0};
	uint32_t i = 0;

	s_search_ctrl.cur_server_stream_total = app_get_current_server_stream_num();
	if((s_search_ctrl.cur_stream_id_arry == NULL) || (s_search_ctrl.cur_server_stream_total > max_arry_num))
	{
		APP_FREE(s_search_ctrl.cur_stream_id_arry);
		s_search_ctrl.cur_stream_id_arry = GxCore_Calloc(s_search_ctrl.cur_server_stream_total, sizeof(uint32_t));
		max_arry_num = s_search_ctrl.cur_server_stream_total;
		if(s_search_ctrl.cur_stream_id_arry == NULL)
			return GXCORE_ERROR;
	}
	else
	{
		memset(s_search_ctrl.cur_stream_id_arry, 0, s_search_ctrl.cur_server_stream_total);
	}
	for(i = 0; i < s_search_ctrl.cur_server_stream_total; i++)
	{
		node_tp.node_type = NODE_TP;
		node_tp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
		s_search_ctrl.cur_stream_id_arry[i] = node_tp.tp_data.id;
	}
	return GXCORE_SUCCESS;
}

static int _app_net_search_poplist_cb(int32_t ret_sel, unsigned short key)
{
    int sel = -1;

    sel = ret_sel ;
    if(sel != -1)
        s_search_ctrl.cur_net_server_sel = sel;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemProperty.itemPropertyCmb.sel = s_search_ctrl.cur_net_server_sel;
    app_cst_item_data_update(ITEM_NET_SERVER_NAME);

    poplist_destory_cb_free_item_content();
    return 0;
}

static int app_create_server_name_list(int sel)
{
	PopList pop_list;
	char **server_name = NULL;
	GxMsgProperty_NodeByIdGet node;
	uint8_t i = 0;
	int ret = -1;

	if(s_search_ctrl.net_server_total <= 0)
	{
		app_log_error("!!!!!!!!!!:server total is empty \n");
		return -1;
	}

	memset(&pop_list, 0, sizeof(PopList));
	server_name = (char**)GxCore_Calloc(s_search_ctrl.net_server_total, sizeof(char*));
	if(server_name == NULL)
	{
        app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
		return -1;
	}
	for(i = 0; i < s_search_ctrl.net_server_total; i++)
    {
        node.node_type = NODE_SAT;
        node.id = s_search_ctrl.net_server_id_arry[i]; // map pos.
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

        server_name[i] = (char*)GxCore_Calloc(1, MAX_SAT_NAME);
        if(NULL == server_name[i])
        {
            app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
            if(server_name != NULL)
            {
                for(i = 0; i < s_search_ctrl.net_server_total; i++)
                {
                        APP_FREE(server_name[i]);
                }
                APP_FREE(server_name);
            }
            return -1;
        }
        strcpy(server_name[i] , (char*)node.sat_data.sat_net.server_name);
    }

	pop_list.item_content = server_name;
	pop_list.sel = sel;
	pop_list.title = STR_ID_NET_SERVER_NAME;
	pop_list.show_num = true;
	pop_list.item_num = s_search_ctrl.net_server_total;
	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _app_net_search_poplist_cb;
	ret = poplist_create(&pop_list);
	return ret;
}

static int app_net_server_ctrl_rebuild(void)
{
	app_net_search_setting_data_init();
    app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemProperty.itemPropertyCmb.content = s_search_ctrl.net_server_name_content;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemProperty.itemPropertyCmb.sel = s_search_ctrl.cur_net_server_sel;
	app_get_current_net_server_url(&s_search_ctrl.cur_net_server_url);
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_server_url;
	app_set_cur_stream_sel(0);
    app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_stream_url;
#if DVB_SERVER_SEARCH_SUPPORT
	if(strlen(s_search_ctrl.cur_server_user_name) == 0)
		thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
		thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_user_name;
	if(strlen(s_search_ctrl.cur_server_password) == 0)
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_password;

	app_cst_item_data_update(ITEM_NET_USER_NAME);
	app_cst_item_data_update(ITEM_NET_PASSWORD);
#endif
	app_cst_item_data_update(ITEM_NET_SERVER_NAME);
	app_cst_item_data_update(ITEM_NET_SERVER_URL);
	app_cst_item_data_update(ITEM_NET_STREAM_URL);
	app_net_search_setting_item_change(s_search_ctrl.cur_item);
	return 0;
}

static int app_net_search_name_cmb_change(int sel)
{
	s_search_ctrl.cur_net_server_sel = sel;
	if(s_search_ctrl.net_server_total != 0)
		s_search_ctrl.cur_net_server_id = s_search_ctrl.net_server_id_arry[s_search_ctrl.cur_net_server_sel];
    app_get_current_net_server_url(&s_search_ctrl.cur_net_server_url);
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_server_url;

	app_build_stream_id_arry();
	app_set_cur_stream_sel(0);
    app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_stream_url;

#if DVB_SERVER_SEARCH_SUPPORT
	app_get_server_user_name(s_search_ctrl.cur_server_user_name);
	if(strlen(s_search_ctrl.cur_server_user_name) == 0)
		thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
	thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_user_name;

	app_get_server_password(s_search_ctrl.cur_server_password);
	if(strlen(s_search_ctrl.cur_server_password) == 0)
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_password;

	app_cst_item_data_update(ITEM_NET_USER_NAME);
	app_cst_item_data_update(ITEM_NET_PASSWORD);
#endif

	if(GXCORE_SUCCESS == app_get_cur_net_server_name(s_search_ctrl.cur_net_server_name))
	{
		thiz_Net_SearchOpt.content_show = true;
		thiz_Net_SearchOpt.Content = s_search_ctrl.cur_net_server_name;
		app_cst_content_data_update();
	}

	app_cst_item_data_update(ITEM_NET_SERVER_URL);
	app_cst_item_data_update(ITEM_NET_STREAM_URL);

	return EVENT_TRANSFER_KEEPON;
}

static int app_net_search_name_cmb_keypress(void)
{
	CSTItemData combox_data = {0};

	app_cst_item_data_get(ITEM_NET_SERVER_NAME, &combox_data);
	app_create_server_name_list(combox_data.value);
	return EVENT_TRANSFER_KEEPON;
}

static int _net_server_url_keypress(void)
{

	return EVENT_TRANSFER_KEEPON;
}


static int _net_stream_url_keypress(void)
{

	return EVENT_TRANSFER_KEEPON;
}

static int _net_fta_cmb_change(int sel)
{
	s_search_ctrl.fta_mode = sel;
	return EVENT_TRANSFER_KEEPON;
}

static int _net_search_start_change(int sel)
{
	s_search_ctrl.search_mode = sel;
	return EVENT_TRANSFER_KEEPON;
}

static status_t app_server_list_del(void)
{
    status_t ret = GXCORE_ERROR;
	uint32_t del_num = 0;
	uint32_t *id_array = NULL;

    del_num = s_search_ctrl.serverIdOps->id_total_get(s_search_ctrl.serverIdOps);
    if(del_num > 0)
    {
        id_array = GxCore_Calloc(del_num, sizeof(uint32_t));
        if(id_array != NULL)
        {
            s_search_ctrl.serverIdOps->id_get(s_search_ctrl.serverIdOps, (uint32_t*)(id_array));
			ret = app_delete_net_server(id_array, del_num);
            GxCore_Free(id_array);
        }
    }

    return ret;
}

static int _delete_sure_yes_no(PopDlgRet ret)
{

    if(ret == POP_VAL_OK)
	{
		if(s_search_ctrl.list_type == SERVER_TYPE)
		{
			app_server_list_del();
		}
		else if(s_search_ctrl.list_type == STREAM_TYPE)
		{
			app_net_stream_list_del();
		}
		app_net_search_setting_data_init();
		app_csl_list_updata();
	}
	return 0;
}

static status_t app_net_stream_list_del(void)
{
    status_t ret = GXCORE_ERROR;
	uint32_t *id_array = NULL;
	uint32_t del_num = 0;

    del_num = s_search_ctrl.streamIdOps->id_total_get(s_search_ctrl.streamIdOps);
    if(del_num > 0)
    {
        id_array = GxCore_Calloc(del_num, sizeof(uint32_t));
        if(id_array != NULL)
        {
            s_search_ctrl.streamIdOps->id_get(s_search_ctrl.streamIdOps, (uint32_t*)(id_array));
			ret = app_delete_net_stream(id_array, del_num);
            GxCore_Free(id_array);
        }
    }
    return ret;
}

static void app_url_list_para_init(int list_sel, char* title, int (*ListCreateCb)(void), int (*ListKeypressCb)(int, int),
		int (*ListGetTotalCb)(void), int (*ListGetDataCb)(void *usrdata), int (*ListExitCb)(void))
{
	memset(&s_search_ctrl.Listpara, 0, sizeof(CSLOps));
	s_search_ctrl.Listpara.ListSel = list_sel;
	s_search_ctrl.Listpara.ListTitle = title;
	s_search_ctrl.Listpara.CreateCb = ListCreateCb;
	s_search_ctrl.Listpara.KeypressCb = ListKeypressCb;
	s_search_ctrl.Listpara.GetTotalCb = ListGetTotalCb;
	s_search_ctrl.Listpara.GetDataCb = ListGetDataCb;
	s_search_ctrl.Listpara.ExitCb = ListExitCb;
}

static int app_get_server_delete_list_total(void)
{
	if(s_search_ctrl.list_type == SERVER_TYPE)
	{
		return s_search_ctrl.net_server_total;
	}
	else if(s_search_ctrl.list_type == STREAM_TYPE)
	{
		return s_search_ctrl.cur_server_stream_total;
	}
	else
		return 0;
}

static int app_get_stream_list_total(void)
{
	return s_search_ctrl.cur_server_stream_total;
}

char *url_buffer = NULL;
static int app_get_server_delete_list_data(void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
	static char buffer1[MAX_LEN];
	static GxMsgProperty_NodeByIdGet node = {0};

    item = (ListItemPara*)usrdata;
    if((NULL == item) ||(0 > item->sel))
        return GXCORE_ERROR;

	if(url_buffer != NULL)
	{
		memset(url_buffer, 0, APP_MAX_NET_URL_LEN);
	}
	else
	{
		url_buffer = GxCore_Calloc(APP_MAX_NET_URL_LEN, sizeof(char));
		if(url_buffer == NULL)
			return GXCORE_ERROR;
	}
	item->x_offset = 0;
    if (s_search_ctrl.serverIdOps->id_check(s_search_ctrl.serverIdOps, item->sel) == 0)//ID_UNEXIST
        item->image = NULL;
    else
        item->image = "s_choice";
    item->string = NULL;

    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;

    item = item->next;
    item->x_offset = 2;
    node.node_type = NODE_SAT;
    node.id = s_search_ctrl.net_server_id_arry[item->sel];
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    item->string = (char*)node.sat_data.sat_net.server_name;

    item = item->next;
    item->x_offset = 2;
	uint32_t server_index = 0;
	server_index = app_get_server_index_by_sat_id(s_search_ctrl.net_server_id_arry[item->sel]);
	if(server_index == 0)
	{
		item->string = STR_ID_NONE;
	}
	else
	{
		GxBus_PmGetNetServerUrlByIndex(&server_index, &url_buffer, 1);
		item->string = url_buffer;
	}
	return GXCORE_SUCCESS;
}

static int app_get_stream_delete_list_data(void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
	uint32_t server_index = 0;
	static char buffer1[MAX_LEN];

    item = (ListItemPara*)usrdata;
    if((NULL == item) ||(0 > item->sel))
        return GXCORE_ERROR;

	if(url_buffer != NULL)
	{
		memset(url_buffer, 0, APP_MAX_NET_URL_LEN);
	}
	else
	{
		url_buffer = GxCore_Calloc(APP_MAX_NET_URL_LEN, sizeof(char));
		if(url_buffer == NULL)
			return GXCORE_ERROR;
	}

	item->x_offset = 0;
	if (s_search_ctrl.streamIdOps->id_check(s_search_ctrl.streamIdOps, item->sel) == 0)//ID_UNEXIST
        item->image = NULL;
    else
        item->image = "s_choice";
    item->string = NULL;

    //col-1: num
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;

    //col-2: name
    item = item->next;
	item->x_offset = 2;
    item->string = s_search_ctrl.cur_net_server_name;

	//get url
    item = item->next;
	item->x_offset = 2;
	server_index = _get_stream_index_by_stream_id(s_search_ctrl.cur_stream_id_arry[item->sel]);
	if(server_index == 0)
	{
		item->string = STR_ID_NONE;
	}
	else
	{
		GxBus_PmGetNetStreamUrlByIndex(&server_index, &url_buffer, 1);
		item->string = url_buffer;
	}
	return GXCORE_SUCCESS;
}

static int app_get_stream_list_data(void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
	uint32_t server_index = 0;
	static char buffer1[MAX_LEN];

    item = (ListItemPara*)usrdata;
    if((NULL == item) ||(0 > item->sel))
        return GXCORE_ERROR;

	if(url_buffer != NULL)
	{
		memset(url_buffer, 0, APP_MAX_NET_URL_LEN);
	}
	else
	{
		url_buffer = GxCore_Calloc(APP_MAX_NET_URL_LEN, sizeof(char));
		if(url_buffer == NULL)
			return GXCORE_ERROR;
	}

	item->x_offset = 0;
    item->image = NULL;

    item->string = NULL;
    //col-1: num
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;

    //col-2: name
    item = item->next;
	item->x_offset = 2;
    item->string = s_search_ctrl.cur_net_server_name;

	//get url
    item = item->next;
	item->x_offset = 2;
	server_index = _get_stream_index_by_stream_id(s_search_ctrl.cur_stream_id_arry[item->sel]);
	if(server_index == 0)
	{
		item->string = STR_ID_NONE;
	}
	else
	{
		GxBus_PmGetNetStreamUrlByIndex(&server_index, &url_buffer, 1);
		item->string = url_buffer;
	}
	return GXCORE_SUCCESS;


}

static int app_stream_list_keypress(int key_value, int list_sel)
{
	switch(key_value)
	{
		case STBK_OK:
			if(s_search_ctrl.list_type == STREAM_TYPE)
			{
				app_set_cur_stream_sel(list_sel);
				app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
                GUI_EndDialog(WND_SERVER_LIST);
                GUI_SetInterface("flush", NULL);
			}
			break;
	}
	return GXCORE_SUCCESS;
}

static int app_server_delete_list_keypress(int key_value, int list_sel)
{
	PopDlg pop;
	uint32_t del_num = 0;
	switch(key_value)
	{
		case STBK_OK:
			if(s_search_ctrl.list_type == SERVER_TYPE)
			{
				if(list_sel > s_search_ctrl.net_server_total)
					return -1;
				if (s_search_ctrl.serverIdOps->id_check(s_search_ctrl.serverIdOps, list_sel) == 0)//ID_UNEXIST
				{
            	    s_search_ctrl.serverIdOps->id_add(s_search_ctrl.serverIdOps, list_sel, s_search_ctrl.net_server_id_arry[list_sel]);
            	}
            	else
            	{
            	    s_search_ctrl.serverIdOps->id_delete(s_search_ctrl.serverIdOps, list_sel);
            	}
			}
			else if(s_search_ctrl.list_type == STREAM_TYPE)
			{
				if(list_sel > s_search_ctrl.cur_server_stream_total)
					return -1;
				if (s_search_ctrl.streamIdOps->id_check(s_search_ctrl.streamIdOps, list_sel) == 0)//ID_UNEXIST
				{
            	    s_search_ctrl.streamIdOps->id_add(s_search_ctrl.streamIdOps, list_sel, s_search_ctrl.cur_stream_id_arry[list_sel]);
            	}
            	else
            	{
            	    s_search_ctrl.streamIdOps->id_delete(s_search_ctrl.streamIdOps, list_sel);
            	}
			}
			app_csl_list_row_updata(list_sel);
			break;
		case STBK_RED:
			if(s_search_ctrl.list_type == SERVER_TYPE)
				del_num = s_search_ctrl.serverIdOps->id_total_get(s_search_ctrl.serverIdOps);
			else if(s_search_ctrl.list_type == STREAM_TYPE)
				del_num = s_search_ctrl.streamIdOps->id_total_get(s_search_ctrl.streamIdOps);
			if(del_num <= 0)
				break;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.mode = POP_MODE_UNBLOCK;
			pop.format = POP_FORMAT_DLG;
       		pop.str = STR_ID_SURE_DELETE;
       		pop.title = STR_ID_DELETE;
       		pop.exit_cb = _delete_sure_yes_no;
       		popdlg_create(&pop);
			break;
	}
	return GXCORE_SUCCESS;
}

static int app_server_delete_list_build(void)
{
	if(s_search_ctrl.list_type == SERVER_TYPE)
	{
		s_search_ctrl.serverIdOps = new_id_ops();
		if(s_search_ctrl.serverIdOps == NULL)
			return GXCORE_ERROR;
        s_search_ctrl.serverIdOps->id_open(s_search_ctrl.serverIdOps, MANAGE_SAME_LIST, s_search_ctrl.net_server_total);
	}
	else if(s_search_ctrl.list_type == STREAM_TYPE)
	{
		s_search_ctrl.streamIdOps = new_id_ops();
		if(s_search_ctrl.streamIdOps == NULL)
			return GXCORE_ERROR;
        s_search_ctrl.streamIdOps->id_open(s_search_ctrl.streamIdOps, MANAGE_SAME_LIST, s_search_ctrl.cur_server_stream_total);
	}
	else
	{
	}
	return GXCORE_SUCCESS;
}

static int app_server_delete_list_exit(void)
{
	if(s_search_ctrl.serverIdOps != NULL)
        del_id_ops(s_search_ctrl.serverIdOps);
    if(s_search_ctrl.streamIdOps != NULL)
        del_id_ops(s_search_ctrl.streamIdOps);
	APP_FREE(url_buffer);
	return GXCORE_SUCCESS;
}

static int app_stream_list_exit(void)
{
	APP_FREE(url_buffer);
	return GXCORE_SUCCESS;
}

static status_t app_net_get_server_search_para(void)
{
	static uint32_t *sp_ts_src = NULL;
	status_t ret = GXCORE_SUCCESS;
	AppFrontend_Config cfg = {0};
	int i = 0;
	extern status_t _modify_prog_callback(GxBusPmDataProg* prog);
	extern private_parse_status _pmt_private_function(uint8_t *p_section_data, uint32_t len);
	extern uint32_t  _check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid);

	APP_FREE(sp_ts_src);
	sp_ts_src = (uint32_t*)GxCore_Calloc(1, s_search_ctrl.cur_server_stream_total * sizeof(uint32_t));
	if (sp_ts_src == NULL)
		return -1;

	memset(&s_NetSearch, 0, sizeof(GxMsgProperty_ScanNetStart));
	s_NetSearch.scan_type = GX_SEARCH_MANUAL;
	s_NetSearch.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	if(s_search_ctrl.fta_mode == FTA_YES)
		s_NetSearch.fta_cas = GX_SEARCH_FTA;
	else
		s_NetSearch.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	s_NetSearch.nit_switch = GX_SEARCH_NIT_DISABLE;
	s_NetSearch.demux_id = 0;
	s_NetSearch.params_net.max_num = s_search_ctrl.cur_server_stream_total;
	app_ioctl(APP_MULTI_DEMOD_NUM, FRONTEND_CONFIG_GET, &cfg);
	for(i = 0; i < s_search_ctrl.cur_server_stream_total; i++)
		sp_ts_src[i] = cfg.ts_src;
	s_NetSearch.params_net.ts = sp_ts_src;
	s_NetSearch.params_net.array = s_search_ctrl.cur_stream_id_arry;
	s_NetSearch.ext = NULL;
	s_NetSearch.ext_num = 0;
	s_NetSearch.time_out.pat = TIMEOUT_PAT;
	s_NetSearch.time_out.pmt = TIMEOUT_PMT;
	s_NetSearch.time_out.sdt = TIMEOUT_SDT;
	s_NetSearch.time_out.nit = TIMEOUT_NIT;
	s_NetSearch.modify_prog = _modify_prog_callback;
	s_NetSearch.pmt_parse_fun = _pmt_private_function;
	s_NetSearch.check_ca_fun = _check_ca_system;

	return ret;

}

uint32_t s_sp_ts_src = DEMUX_SDRAM;
static status_t _net_get_stream_search_para(void)
{
	status_t ret = GXCORE_SUCCESS;
	AppFrontend_Config cfg = {0};
	extern status_t _modify_prog_callback(GxBusPmDataProg* prog);
	extern private_parse_status _pmt_private_function(uint8_t *p_section_data, uint32_t len);
	extern uint32_t  _check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid);

	app_ioctl(APP_MULTI_DEMOD_NUM, FRONTEND_CONFIG_GET, &cfg);
    s_sp_ts_src = cfg.ts_src;
	memset(&s_NetSearch, 0, sizeof(GxMsgProperty_ScanNetStart));
	s_NetSearch.scan_type = GX_SEARCH_MANUAL;
	s_NetSearch.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	if(s_search_ctrl.fta_mode == FTA_YES)
		s_NetSearch.fta_cas = GX_SEARCH_FTA;
	else
		s_NetSearch.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	s_NetSearch.nit_switch = GX_SEARCH_NIT_DISABLE;
	s_NetSearch.demux_id = 0;
	s_NetSearch.params_net.sat_id = s_search_ctrl.cur_net_server_id;
	s_NetSearch.params_net.max_num = 1;
	s_NetSearch.params_net.ts = &s_sp_ts_src;
	s_NetSearch.params_net.array = &s_search_ctrl.cur_net_stream_id;
	s_NetSearch.ext = NULL;
	s_NetSearch.ext_num = 0;
	s_NetSearch.time_out.pat = TIMEOUT_PAT;
	s_NetSearch.time_out.pmt = TIMEOUT_PMT;
	s_NetSearch.time_out.sdt = TIMEOUT_SDT;
	s_NetSearch.time_out.nit = TIMEOUT_NIT;
	s_NetSearch.modify_prog = _modify_prog_callback;
	s_NetSearch.pmt_parse_fun = _pmt_private_function;
	s_NetSearch.check_ca_fun = _check_ca_system;
	return ret;
}

static void _net_server_search_start(void)
{
	app_net_get_server_search_para();
	app_search_set_search_type(SEARCH_SERVER);
	GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);
    app_send_msg_exec(GXMSG_SEARCH_SCAN_NET_START, &s_NetSearch);
}

static void _net_stream_search_start(void)
{
	_net_get_stream_search_para();
	app_search_set_search_type(SEARCH_STREAM);
	GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);
    app_send_msg_exec(GXMSG_SEARCH_SCAN_NET_START, &s_NetSearch);
}
static int _net_search_start_keypress(void)
{
	if(s_search_ctrl.net_server_total <= 0)
		return EVENT_TRANSFER_KEEPON;
	if(SEARCH_SEL_SERVER == s_search_ctrl.search_mode)
    {
		_net_server_search_start();
	}
	else if(SEARCH_SEL_STREAM == s_search_ctrl.search_mode)
	{
		_net_stream_search_start();
	}
	return EVENT_TRANSFER_KEEPON;
}


static status_t app_delete_net_server_url_by_server_id(uint32_t *delete_id_arry, uint32_t delete_num)
{
	GxMsgProperty_NodeByPosGet node_stream = {0};
	GxMsgProperty_NodeNumGet node_stream_num = {0};
	status_t ret = GXCORE_ERROR;
	uint16_t stream_num_bak = 0;
	uint32_t *stream_index_arry = NULL;
	uint32_t server_index = 0;
	uint32_t i = 0;
	uint32_t j = 0;

	if(delete_num ==0 || delete_id_arry == NULL)
	{
		ret = GXCORE_ERROR;
		return ret;
	}
	for(i = 0; i < delete_num; i++)
	{
		node_stream_num.node_type = NODE_TP;
		node_stream_num.sat_id = delete_id_arry[i];
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_stream_num);
		if(node_stream_num.node_num <= 0)
		{
			continue;
		}
		if(node_stream_num.node_num <= stream_num_bak)
		{
			memset(stream_index_arry, 0, stream_num_bak);
		}
		else
		{
			APP_FREE(stream_index_arry);
			stream_index_arry = GxCore_Calloc(node_stream_num.node_num, sizeof(uint32_t));
			if(stream_index_arry == NULL)
			{
				ret = GXCORE_ERROR;
				break;
			}
		}
		stream_num_bak = node_stream_num.node_num;
		for(j = 0; j < node_stream_num.node_num; j++)
		{
			node_stream.node_type = NODE_TP;
			node_stream.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_stream);
			stream_index_arry[i] = node_stream.tp_data.tp_net.index;
			app_log_debug("###net search: node_stream.tp_data.tp_net.index is %d\n", node_stream.tp_data.tp_net.index);
		}
		server_index = app_get_server_index_by_sat_id(delete_id_arry[i]);
		ret = GxBus_PmDeleteNetServerUrlByIndex(&server_index, 1);
		if(ret != GXCORE_SUCCESS)
			continue;
		ret = GxBus_PmDeleteNetStreamUrlByIndex(stream_index_arry, node_stream_num.node_num);
		if(ret != GXCORE_SUCCESS)
		{
			APP_FREE(stream_index_arry);
			break;
		}
	}
	APP_FREE(stream_index_arry);
	return ret;
}

static status_t app_delete_net_server(uint32_t *delete_arry, uint32_t delete_num)
{
	GxMsgProperty_NodeDelete node= {0};
    status_t ret = GXCORE_ERROR;

    node.node_type = NODE_SAT;
    node.num = delete_num;
    if(node.num > 0 && delete_arry != NULL)
    {
        node.id_array = GxCore_Malloc(node.num*sizeof(uint32_t));
        if(node.id_array != NULL)
        {
            memcpy(node.id_array, delete_arry, node.num * sizeof(uint32_t));
			app_delete_net_server_url_by_server_id(delete_arry, delete_num);
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
            {
                g_AppBook.invalid_remove();
                ret = GXCORE_SUCCESS;
            }
            APP_FREE(node.id_array);
        }
    }
	return ret;
}

static status_t app_delete_net_stream_url_by_stream_id(uint32_t *delete_arry, uint32_t delete_num)
{
	uint32_t *stream_index_arry = NULL;
	status_t ret = GXCORE_ERROR;
	GxMsgProperty_NodeByPosGet node_stream = {0};
	int i = 0;

	if(delete_num <= 0)
		return GXCORE_ERROR;
	stream_index_arry = GxCore_Calloc(delete_num, sizeof(uint32_t));
	if(stream_index_arry == NULL)
	{
		app_log_error("###net malloc failed!");
		return GXCORE_ERROR;
	}
	for(i = 0; i < delete_num; i++)
	{
		node_stream.node_type = NODE_TP;
		node_stream.id = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_stream);
		stream_index_arry[i] = node_stream.tp_data.tp_net.index;
		app_log_debug("###net search: node_stream.tp_data.tp_net.index is %d\n", node_stream.tp_data.tp_net.index);
	}
	ret = GxBus_PmDeleteNetStreamUrlByIndex(stream_index_arry, delete_num);
	if(ret == GXCORE_ERROR)
		app_log_error("###net stream delete error");
	APP_FREE(stream_index_arry);
	return ret;
}

static status_t app_delete_net_stream(uint32_t *delete_arry, uint32_t delete_num)
{
	GxMsgProperty_NodeDelete node= {0};
    status_t ret = GXCORE_ERROR;

    node.node_type = NODE_TP;
    node.num = delete_num;
    if(node.num > 0 && delete_arry != NULL)
    {
        node.id_array = GxCore_Malloc(node.num*sizeof(uint32_t));
        if(node.id_array != NULL)
        {
            memcpy(node.id_array, delete_arry, delete_num * sizeof(uint32_t));
			app_delete_net_stream_url_by_stream_id(delete_arry, delete_num);
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
            {
                g_AppBook.invalid_remove();
                ret = GXCORE_SUCCESS;
            }
            APP_FREE(node.id_array);
        }
    }
	return ret;
}


static void app_net_search_setting_data_init(void)
{
	APP_FREE(s_search_ctrl.net_server_name_content);
	APP_FREE(s_search_ctrl.cur_net_server_url);
	APP_FREE(s_search_ctrl.cur_net_stream_url);
    app_net_sat_name_build(&s_search_ctrl.net_server_name_content);
	if(s_search_ctrl.net_server_total  != 0)
		s_search_ctrl.cur_net_server_id = s_search_ctrl.net_server_id_arry[s_search_ctrl.cur_net_server_sel];
	app_get_cur_net_server_name(s_search_ctrl.cur_net_server_name);
    app_get_current_net_server_url(&s_search_ctrl.cur_net_server_url);
	app_build_stream_id_arry();
	app_set_cur_stream_sel(0);
    app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
#if DVB_SERVER_SEARCH_SUPPORT
	memset(s_search_ctrl.cur_server_user_name, 0, USER_NAME_LEN);
	memset(s_search_ctrl.cur_server_password, 0, PASSWORD_LEN);
	app_get_server_user_name(s_search_ctrl.cur_server_user_name);
	app_get_server_password(s_search_ctrl.cur_server_password);
#endif
}

static void _net_server_name_item_init(void)
{
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemTitle = STR_ID_NET_SERVER_NAME;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemType = CST_ITEM_POP_CHOICE;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemProperty.itemPropertyCmb.content = s_search_ctrl.net_server_name_content;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemProperty.itemPropertyCmb.sel = s_search_ctrl.cur_net_server_sel;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemCb.cmbCb.CmbChange = app_net_search_name_cmb_change;
    thiz_NetSearchItem[ITEM_NET_SERVER_NAME].itemCb.cmbCb.CmbPress = app_net_search_name_cmb_keypress;
}

static void _net_server_url_item_init(void)
{
	thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemTitle = STR_ID_NET_SERVER_URL;
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemType = CST_ITEM_PUSH;
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_server_url;
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemProperty.itemPropertyBtn.roll_format = true;
    thiz_NetSearchItem[ITEM_NET_SERVER_URL].itemCb.btnCb.BtnPress = _net_server_url_keypress;
}

static void _net_stream_url_item_init(void)
{
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemTitle = STR_ID_NET_STREAM_URL;
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemType = CST_ITEM_PUSH;
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_net_stream_url;
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemProperty.itemPropertyBtn.roll_format = true;
    thiz_NetSearchItem[ITEM_NET_STREAM_URL].itemCb.btnCb.BtnPress = _net_stream_url_keypress;
}

#if DVB_SERVER_SEARCH_SUPPORT
static void _net_user_name_item_init(void)
{
	app_get_server_user_name(s_search_ctrl.cur_server_user_name);
	thiz_NetSearchItem[ITEM_NET_USER_NAME].itemStatus = CST_ITEM_NORMAL;
    thiz_NetSearchItem[ITEM_NET_USER_NAME].itemTitle = STR_ID_USERNAME;
    thiz_NetSearchItem[ITEM_NET_USER_NAME].itemType = CST_ITEM_PUSH;
	if(strlen(s_search_ctrl.cur_server_user_name) == 0)
		thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
		thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_user_name;
    thiz_NetSearchItem[ITEM_NET_USER_NAME].itemProperty.itemPropertyBtn.roll_format = false;
    thiz_NetSearchItem[ITEM_NET_USER_NAME].itemCb.btnCb.BtnPress = NULL;
}

static void _net_password_item_init(void)
{
	app_get_server_password(s_search_ctrl.cur_server_password);
	thiz_NetSearchItem[ITEM_NET_PASSWORD].itemStatus = CST_ITEM_NORMAL;
    thiz_NetSearchItem[ITEM_NET_PASSWORD].itemTitle = STR_ID_PASSWD;
    thiz_NetSearchItem[ITEM_NET_PASSWORD].itemType = CST_ITEM_PUSH;
	if(strlen(s_search_ctrl.cur_server_password) == 0)
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = STR_ID_NONE;
	else
		thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.string = s_search_ctrl.cur_server_password;
    thiz_NetSearchItem[ITEM_NET_PASSWORD].itemProperty.itemPropertyBtn.roll_format = false;
    thiz_NetSearchItem[ITEM_NET_PASSWORD].itemCb.btnCb.BtnPress = NULL;
}

static char *file_path_bak = NULL;
static char *file_path = NULL;

static int _app_net_search_server_filedlg_exit_cb(WndStatus ret)
{
    int str_len = 0;
    PopDlg pop;

    if (ret == WND_OK)
    {
        memset(&pop, 0, sizeof(PopDlg));
        app_get_file_real_path(&file_path);
        if (file_path != NULL)
        {
            APP_FREE(file_path_bak);
            str_len = strlen(file_path);
            file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if (file_path_bak != NULL)
            {
                memcpy(file_path_bak, file_path, str_len);
                file_path_bak[str_len] = '\0';
            }

            if (strcasecmp(file_path+str_len-5,".json") == 0 || strcasecmp(file_path+str_len-5,".JSON") == 0)
            {
                app_log_debug("import path:%s\n", file_path);
                if(GXCORE_SUCCESS != app_add_net_server_from_json(file_path))
                {
                    pop.str = STR_ID_IMPORT_ERROR;
                }
                else
                {
                    pop.str = STR_ID_IMPORT_SUCCESS;
                }
            }
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 3;
            if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
                popdlg_create(&pop);
        }
    }

    app_net_server_ctrl_rebuild();
    return 0 ;
}

static void app_net_search_import_server(void)
{
#define UPDATE_FILE_SUFFFIX    "json;JSON"
    FileListParam file_para;

    if (GXCORE_FILE_EXIST != GxCore_FileExists(file_path_bak))
        APP_FREE(file_path_bak);

    memset(&file_para, 0, sizeof(file_para));
    file_para.cur_path = file_path_bak;
    file_para.dest_path = &file_path;
    file_para.suffix = UPDATE_FILE_SUFFFIX;
    file_para.dest_mode = DEST_MODE_FILE;
    file_para.exit_cb = _app_net_search_server_filedlg_exit_cb;
    app_get_file_path_dlg(&file_para);
}


static int app_server_search_setting_blue_key(void)
{
	PopDlg pop;
	if (check_usb_status() == false)
    {
		memset(&pop, 0, sizeof(PopDlg));
        pop.str = STR_ID_INSERT_USB;
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec= 3;
        if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
            popdlg_create(&pop);
		return GXCORE_ERROR;
    }
	app_net_search_import_server();
	return GXCORE_SUCCESS;
}
#endif

static void  _net_fat_item_init(void)
{
	thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemTitle = STR_ID_FTA_ONLY;
    thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemType = CST_ITEM_CHOICE;
    thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemProperty.itemPropertyCmb.content = YES_NO_CONTENT;
    thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemProperty.itemPropertyCmb.sel = s_search_ctrl.fta_mode;
    thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemCb.cmbCb.CmbChange = _net_fta_cmb_change;
    thiz_NetSearchItem[ITEM_NET_FTA_ONLY].itemCb.cmbCb.CmbPress = NULL;
}

static void _net_start_search_item_init(void)
{
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemTitle = STR_ID_PRESET_SEARCH;
	thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemStatus = CST_ITEM_NORMAL;
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemType = CST_ITEM_POP_CHOICE;
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemProperty.itemPropertyCmb.content = s_search_ctrl.net_search_start_content;
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemProperty.itemPropertyCmb.sel = s_search_ctrl.search_mode;
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemCb.cmbCb.CmbChange = _net_search_start_change;
    thiz_NetSearchItem[ITEM_NET_SEARCH_START].itemCb.cmbCb.CmbPress = _net_search_start_keypress;
}

static int app_net_search_setting_exit_cb(CSTExitType exit_type)
{
    APP_FREE(s_search_ctrl.net_server_name_content);
	APP_FREE(s_search_ctrl.cur_net_server_url);
	APP_FREE(s_search_ctrl.cur_net_stream_url);
	APP_FREE(s_search_ctrl.net_server_id_arry);
	APP_FREE(s_search_ctrl.cur_stream_id_arry);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);

	return GXCORE_SUCCESS;
}

static void app_build_mult_key_show(char *red_string, int (*RedKeyFun)(void),char *green_string, int (*greenKeyFun)(void),
		char *yellow_string, int (*yellowKeyFun)(void), char *blue_string, int (*blueKeyFun)(void))
{
	thiz_Net_SearchOpt.funckey.redKey.keyName = red_string;
	thiz_Net_SearchOpt.funckey.greenKey.keyName = green_string;
    thiz_Net_SearchOpt.funckey.yellowKey.keyName = yellow_string;
	thiz_Net_SearchOpt.funckey.blueKey.keyName = blue_string;
    thiz_Net_SearchOpt.funckey.redKey.keyPressFun = RedKeyFun;
    thiz_Net_SearchOpt.funckey.greenKey.keyPressFun = greenKeyFun;
    thiz_Net_SearchOpt.funckey.yellowKey.keyPressFun = yellowKeyFun;
	thiz_Net_SearchOpt.funckey.blueKey.keyPressFun = blueKeyFun;

}

static int app_net_search_setting_item_change(int sel)
{
	s_search_ctrl.list_type = DEF_TYPE;
	if(s_search_ctrl.net_server_total > 0)
	{
		if(sel == ITEM_NET_SERVER_URL)
		{
			s_search_ctrl.list_type = SERVER_TYPE;
			thiz_Net_SearchOpt.Content = s_search_ctrl.cur_net_server_url;
			thiz_Net_SearchOpt.content_show = true;
			app_build_mult_key_show(STR_ID_DELETE, app_net_search_setting_red_key, NULL, NULL,
					STR_ID_EDIT, app_net_search_setting_yellow_key,STR_ID_IMPORT, app_net_search_setting_blue_key);

		}
		else if(sel == ITEM_NET_STREAM_URL)
		{
			s_search_ctrl.list_type = STREAM_TYPE;
			thiz_Net_SearchOpt.content_show = true;
			thiz_Net_SearchOpt.Content = s_search_ctrl.cur_net_stream_url;
			app_build_mult_key_show(STR_ID_DELETE, app_net_search_setting_red_key, STR_ID_NET_STREAM_LIST, app_net_search_setting_green_key,
					STR_ID_EDIT, app_net_search_setting_yellow_key,STR_ID_IMPORT, app_net_search_setting_blue_key);

		}
		else if(sel == ITEM_NET_SERVER_NAME)
		{
			s_search_ctrl.list_type = SERVER_TYPE;
			thiz_Net_SearchOpt.content_show = true;
			app_get_cur_net_server_name(s_search_ctrl.cur_net_server_name);
			thiz_Net_SearchOpt.Content = s_search_ctrl.cur_net_server_name;
			app_build_mult_key_show(STR_ID_DELETE, app_net_search_setting_red_key, NULL, NULL,
					STR_ID_EDIT, app_net_search_setting_yellow_key,STR_ID_IMPORT, app_net_search_setting_blue_key);
		}
#if DVB_SERVER_SEARCH_SUPPORT
		else if(sel == ITEM_NET_PASSWORD || sel == ITEM_NET_USER_NAME)
		{
			thiz_Net_SearchOpt.content_show = false;
			thiz_Net_SearchOpt.Content = " ";
			app_build_mult_key_show(NULL, NULL , NULL, NULL,
					STR_ID_EDIT, app_net_search_setting_yellow_key, STR_ID_IMPORT, app_net_search_setting_blue_key);
		}
		else
		{
			thiz_Net_SearchOpt.content_show = false;
			thiz_Net_SearchOpt.Content = " ";
			app_build_mult_key_show(NULL, NULL, NULL, NULL,
					NULL, NULL, STR_ID_IMPORT, app_net_search_setting_blue_key);
		}
#endif
	}
	else
	{
		thiz_Net_SearchOpt.content_show = false;
		thiz_Net_SearchOpt.Content = " ";
		app_build_mult_key_show(NULL, NULL, NULL, NULL,
				NULL, NULL,STR_ID_IMPORT, app_net_search_setting_blue_key);
	}
	s_search_ctrl.cur_item = sel;
	app_cst_content_data_update();
	app_cst_key_data_updata();
	return EVENT_TRANSFER_KEEPON;
}
static int app_net_search_setting_item_got_focus(void)
{
	if(s_search_ctrl.ctrl_mode == MODE_DEL || s_search_ctrl.ctrl_mode == MODE_ADD)
		app_net_server_ctrl_rebuild();
	else
	{
		if(s_search_ctrl.cur_item == ITEM_NET_STREAM_URL)
		{
			app_get_current_stream_url(&s_search_ctrl.cur_net_stream_url);
		}
		else if(s_search_ctrl.cur_item == ITEM_NET_SERVER_URL)
		{
			app_get_current_net_server_url(&s_search_ctrl.cur_net_server_url);
		}
#if DVB_SERVER_SEARCH_SUPPORT
		else if(s_search_ctrl.cur_item == ITEM_NET_USER_NAME)
		{
			app_get_server_user_name(s_search_ctrl.cur_server_user_name);
		}
		else if(s_search_ctrl.cur_item == ITEM_NET_PASSWORD)
		{
			app_get_server_password(s_search_ctrl.cur_server_password);
		}
#endif
		app_cst_item_data_update(s_search_ctrl.cur_item);
		app_net_search_setting_item_change(s_search_ctrl.cur_item);
	}
	s_search_ctrl.ctrl_mode = MODE_DEF;
	return 0;
}

static int app_net_search_setting_green_key(void)
{
	if(s_search_ctrl.list_type == STREAM_TYPE)
	{
		if(s_search_ctrl.cur_server_stream_total <= 0)
			return -1;

		app_build_mult_key_show(NULL,NULL , NULL, NULL,
				NULL, NULL, NULL, NULL );
		app_cst_key_data_updata();

		thiz_Net_SearchOpt.content_show = false;
		thiz_Net_SearchOpt.Content = " ";
		app_cst_content_data_update();
		app_url_list_para_init(s_search_ctrl.cur_net_stream_sel,STR_ID_NET_STREAM_LIST,
				NULL,
				app_stream_list_keypress,
				app_get_stream_list_total,
				app_get_stream_list_data,
				app_stream_list_exit);
		s_search_ctrl.ctrl_mode = MODE_DEF;
		app_csl_dialog_create(&s_search_ctrl.Listpara);
	}

	return GXCORE_SUCCESS;
}

static int app_net_search_setting_red_key(void)
{
	if(s_search_ctrl.list_type == SERVER_TYPE || s_search_ctrl.list_type == STREAM_TYPE)
	{
		if(s_search_ctrl.cur_server_stream_total <= 0 || s_search_ctrl.net_server_total <= 0)
			return -1;

		thiz_Net_SearchOpt.funckey.greenKey.keyName = NULL;
    	thiz_Net_SearchOpt.funckey.yellowKey.keyName = NULL;
    	thiz_Net_SearchOpt.funckey.blueKey.keyName = NULL;
    	thiz_Net_SearchOpt.funckey.greenKey.keyPressFun = NULL;
    	thiz_Net_SearchOpt.funckey.yellowKey.keyPressFun = NULL;
    	thiz_Net_SearchOpt.funckey.blueKey.keyPressFun = NULL;
		app_build_mult_key_show(STR_ID_DELETE, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL);
		app_cst_key_data_updata();

		thiz_Net_SearchOpt.content_show = false;
		thiz_Net_SearchOpt.Content = " ";
		app_cst_content_data_update();
		if(s_search_ctrl.list_type == SERVER_TYPE)
		{
			app_url_list_para_init(s_search_ctrl.cur_net_server_sel, STR_ID_NET_SERVER_LIST,
					app_server_delete_list_build,
					app_server_delete_list_keypress,
					app_get_server_delete_list_total,
					app_get_server_delete_list_data,
					app_server_delete_list_exit);
		}
		else if(s_search_ctrl.list_type == STREAM_TYPE)
		{
			app_url_list_para_init(s_search_ctrl.cur_net_stream_sel, STR_ID_NET_STREAM_LIST,
					app_server_delete_list_build,
					app_server_delete_list_keypress,
					app_get_server_delete_list_total,
					app_get_stream_delete_list_data,
					app_server_delete_list_exit);
		}
		s_search_ctrl.ctrl_mode = MODE_DEL;
		app_csl_dialog_create(&s_search_ctrl.Listpara);
	}
	return GXCORE_SUCCESS;
}

static void app_server_rename_release_cb(PopKeyboard *data)
{
	GxMsgProperty_NodeModify NodeModify;
	GxMsgProperty_NodeByIdGet node = {0};

	if(strcmp(data->out_name, s_search_ctrl.cur_net_server_name) == 0)
		return;
	node.node_type = NODE_SAT;
	node.id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    memcpy(&NodeModify.sat_data, &node.sat_data, sizeof(GxBusPmDataSat));
	NodeModify.node_type = NODE_SAT;
    memset((char *)NodeModify.sat_data.sat_net.server_name, 0, MAX_SAT_NAME);
    memcpy((char *)NodeModify.sat_data.sat_net.server_name, data->out_name, MAX_SAT_NAME - 1);
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
	{
		app_net_server_ctrl_rebuild();
	}

}

static void app_server_url_release_cb(PopKeyboard *data)
{
	int server_index = 0;
	if(strcmp(data->out_name, s_search_ctrl.cur_net_server_url) == 0)
		return;
	server_index = app_get_server_index_by_sat_id(s_search_ctrl.cur_net_server_id);
	if(GXCORE_SUCCESS == GxBus_PmModifyNetServerUrlByIndex(server_index, data->out_name))
		app_net_server_ctrl_rebuild();
}

static void app_stream_url_release_cb(PopKeyboard *data)
{
	int stream_index = 0;
	if(strcmp(data->out_name, s_search_ctrl.cur_net_stream_url) == 0)
		return;
	stream_index = _get_stream_index_by_stream_id(s_search_ctrl.cur_net_stream_id);
	if(GXCORE_SUCCESS == GxBus_PmModifyNetStreamUrlByIndex(stream_index, data->out_name))
		app_net_server_ctrl_rebuild();
}

#if DVB_SERVER_SEARCH_SUPPORT
static bool app_get_server_user_name(char *user_name)
{
	GxMsgProperty_NodeByIdGet node_server = {0};

	if(user_name == NULL)
	{
		return false;
	}

	node_server.node_type = NODE_SAT;
	node_server.id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_server);
	memset(user_name, 0, USER_NAME_LEN);
	strcpy(user_name, (char*)node_server.sat_data.sat_net.user_name);
	return true;
}

static bool app_get_server_password(char *password)
{
	GxMsgProperty_NodeByIdGet node_server = {0};

	if(password == NULL)
	{
		return false;
	}
	node_server.node_type = NODE_SAT;
	node_server.id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_server);
	memset(password, 0, PASSWORD_LEN);
	strcpy(password,(char*)node_server.sat_data.sat_net.password);
	return true;
}

static void app_username_release_cb(PopKeyboard *data)
{
	GxMsgProperty_NodeModify NodeModify;
	GxMsgProperty_NodeByIdGet node = {0};

	if(strcmp(data->out_name, s_search_ctrl.cur_server_user_name) == 0)
		return;
	node.node_type = NODE_SAT;
	node.id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    memcpy(&NodeModify.sat_data, &node.sat_data, sizeof(GxBusPmDataSat));
	NodeModify.node_type = NODE_SAT;
    memset((char *)NodeModify.sat_data.sat_net.user_name, 0, USER_NAME_LEN);
    memcpy((char *)NodeModify.sat_data.sat_net.user_name, data->out_name, USER_NAME_LEN - 1);
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
	{
		app_net_server_ctrl_rebuild();
	}
}

static void app_password_release_cb(PopKeyboard *data)
{
	GxMsgProperty_NodeModify NodeModify;
	GxMsgProperty_NodeByIdGet node = {0};

	if(strcmp(data->out_name, s_search_ctrl.cur_server_password) == 0)
		return;
	node.node_type = NODE_SAT;
	node.id = s_search_ctrl.cur_net_server_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    memcpy(&NodeModify.sat_data, &node.sat_data, sizeof(GxBusPmDataSat));
	NodeModify.node_type = NODE_SAT;
    memset((char *)NodeModify.sat_data.sat_net.password, 0, USER_NAME_LEN);
    memcpy((char *)NodeModify.sat_data.sat_net.password, data->out_name, USER_NAME_LEN - 1);
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
	{
		app_net_server_ctrl_rebuild();
	}
}
#endif

static void app_net_url_cant_edit_info(void)
{
	PopDlg pop;
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG;
	pop.str = STR_URL_TOO_LONG;
    pop.title = NULL;
    pop.mode = POP_MODE_UNBLOCK;
    pop.create_cb = NULL;
    pop.exit_cb = NULL;
    pop.timeout_sec = 2;
    popdlg_create(&pop);
}

#define WND_KEYBOARD_X 360
#define WND_KEYBOARD_Y 130
static PopKeyboard keyboard;
static int app_net_search_setting_yellow_key(void)
{
	if(s_search_ctrl.cur_item == ITEM_NET_SERVER_NAME)
	{
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.pos.x = WND_KEYBOARD_X;
		keyboard.pos.y = WND_KEYBOARD_Y;
		keyboard.in_name = s_search_ctrl.cur_net_server_name;
		keyboard.max_num = MAX_SAT_NAME - 1;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_server_rename_release_cb;
		keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
		multi_language_keyboard_create(&keyboard);
	}
	else if(s_search_ctrl.cur_item == ITEM_NET_SERVER_URL)
	{
		if(strlen(s_search_ctrl.cur_net_server_url) > STR_BUF_LEN)
		{
			app_net_url_cant_edit_info();
			return -1;
		}
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.pos.x = WND_KEYBOARD_X;
		keyboard.pos.y = WND_KEYBOARD_Y;
		keyboard.in_name = s_search_ctrl.cur_net_server_url;
		keyboard.max_num = APP_MAX_NET_URL_LEN - 1;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_server_url_release_cb;
		keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
		multi_language_keyboard_create(&keyboard);
	}
	else if(s_search_ctrl.cur_item == ITEM_NET_STREAM_URL)
	{

		if(strlen(s_search_ctrl.cur_net_stream_url) > STR_BUF_LEN)
		{
			app_net_url_cant_edit_info();
			return -1;
		}
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.pos.x = WND_KEYBOARD_X;
		keyboard.pos.y = WND_KEYBOARD_Y;
		keyboard.in_name = s_search_ctrl.cur_net_stream_url;
		keyboard.max_num = STR_BUF_LEN - 1;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_stream_url_release_cb;
		keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
		multi_language_keyboard_create(&keyboard);
	}
#if DVB_SERVER_SEARCH_SUPPORT
	else if(s_search_ctrl.cur_item == ITEM_NET_USER_NAME)
	{
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.pos.x = WND_KEYBOARD_X;
		keyboard.pos.y = WND_KEYBOARD_Y;
		keyboard.in_name = s_search_ctrl.cur_server_user_name;
		keyboard.max_num = USER_NAME_LEN - 1;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_username_release_cb;
		keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
		multi_language_keyboard_create(&keyboard);
	}
	else if(s_search_ctrl.cur_item == ITEM_NET_PASSWORD)
	{
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.pos.x = WND_KEYBOARD_X;
		keyboard.pos.y = WND_KEYBOARD_Y;
		keyboard.in_name = s_search_ctrl.cur_server_user_name;
		keyboard.max_num = USER_NAME_LEN - 1;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_password_release_cb;
		keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
		multi_language_keyboard_create(&keyboard);
	}
#endif
	return GXCORE_SUCCESS;
}

static int app_net_search_setting_blue_key(void)
{
#if DVB_SERVER_SEARCH_SUPPORT
	return app_server_search_setting_blue_key();
#endif
	return GXCORE_SUCCESS;
}

#if DVB_FILE_SEARCH_SUPPORT

static int _app_net_search_file_filedlg_exit_cb(WndStatus ret)
{
    int str_len = 0;
    PopDlg pop;

    if (ret == WND_OK)
    {
        memset(&pop, 0, sizeof(PopDlg));
        app_get_file_real_path(&file_path);
        if (file_path != NULL)
        {
            APP_FREE(file_path_bak);
            str_len = strlen(file_path);
            file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if (file_path_bak != NULL)
            {
                memcpy(file_path_bak, file_path, str_len);
                file_path_bak[str_len] = '\0';
            }
            if (strcasecmp(file_path + str_len - 3, ".ts") == 0 || strcasecmp(file_path + str_len - 4, ".trp") == 0)
            {
                app_log_debug("import path:%s\n", file_path);
                if(GXCORE_SUCCESS != app_add_ts_path_from_usb(file_path))
                {
                    pop.str = STR_ID_IMPORT_FILE_ERROR;
                }
                else
                {
                    pop.str = STR_ID_IMPORT_SUCCESS;
                }
            }
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 3;
            if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
                popdlg_create(&pop);
        }
    }

    app_net_server_ctrl_rebuild();
    return 0 ;
}

static void app_net_search_import_file(void)
{
#define TS_FILE_SUFFFIX    "ts;TS;trp"
    FileListParam file_para;

    if (GXCORE_FILE_EXIST != GxCore_FileExists(file_path_bak))
        APP_FREE(file_path_bak);

    memset(&file_para, 0, sizeof(file_para));
    file_para.cur_path = file_path_bak;
    file_para.dest_path = &file_path;
    file_para.suffix = TS_FILE_SUFFFIX;
    file_para.dest_mode = DEST_MODE_FILE;
    file_para.exit_cb = _app_net_search_file_filedlg_exit_cb;
    app_get_file_path_dlg(&file_para);
}

static int app_net_search_setting_file_key(void)
{
	PopDlg pop = {0};
	if(s_search_ctrl.cur_item != ITEM_NET_SERVER_NAME)
		return GXCORE_SUCCESS;
	if (check_usb_status() == false)
    {
		memset(&pop, 0, sizeof(PopDlg));
        pop.str = STR_ID_INSERT_USB;
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
            popdlg_create(&pop);
		return GXCORE_ERROR;
    }
	app_net_search_import_file();
	return GXCORE_SUCCESS;
}


#endif

void app_net_search_setting_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
        extern int app_sat2ip_operate_tip_get_force(void);
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif

    memset(&thiz_Net_SearchOpt, 0 ,sizeof(CSTOpt));
    thiz_Net_SearchOpt.menuTitle = STR_ID_NET_SEARCH;
	thiz_Net_SearchOpt.content_show = true;
    thiz_Net_SearchOpt.itemNum = ITEM_NET_SEARCH_TOTAL;
    thiz_Net_SearchOpt.item = thiz_NetSearchItem;
    thiz_Net_SearchOpt.signal_show = false;
	thiz_Net_SearchOpt.Content = " ";
    thiz_Net_SearchOpt.signal.exit_cb = app_net_search_setting_exit_cb;
    thiz_Net_SearchOpt.signal.item_change = app_net_search_setting_item_change;
    thiz_Net_SearchOpt.signal.got_focus = app_net_search_setting_item_got_focus;

	app_net_search_setting_data_init();
	if(s_search_ctrl.net_server_total > 0)
	{
		thiz_Net_SearchOpt.Content = s_search_ctrl.cur_net_server_name;
		thiz_Net_SearchOpt.funckey.redKey.keyName = STR_ID_DELETE;
    	thiz_Net_SearchOpt.funckey.yellowKey.keyName = STR_ID_EDIT;
    	thiz_Net_SearchOpt.funckey.redKey.keyPressFun = app_net_search_setting_red_key;
    	thiz_Net_SearchOpt.funckey.yellowKey.keyPressFun = app_net_search_setting_yellow_key;
	}
#if  DVB_FILE_SEARCH_SUPPORT
	thiz_Net_SearchOpt.funckey.hideKey.keyName = NULL;
    thiz_Net_SearchOpt.funckey.hideKey.keyPressFun = app_net_search_setting_file_key;
#else
    thiz_Net_SearchOpt.funckey.blueKey.keyName = STR_ID_IMPORT;
    thiz_Net_SearchOpt.funckey.blueKey.keyPressFun = app_net_search_setting_blue_key;
#endif
    _net_server_name_item_init();
    _net_server_url_item_init();
    _net_stream_url_item_init();
#if DVB_SERVER_SEARCH_SUPPORT
    _net_user_name_item_init();
    _net_password_item_init();
#endif
    _net_fat_item_init();
    _net_start_search_item_init();

    app_cst_dialog_create(&thiz_Net_SearchOpt);
}

#endif


