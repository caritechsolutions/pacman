#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_keyboard_language.h"
#include "app_windows.h"
#include "app_book.h"

#if NETWORK_SUPPORT && MOD_3G_SUPPORT

#define IMG_OPT_HALF_FOCUS_L			"s_bar_half_focus_l"
#define IMG_OPT_HALF_FOCUS_R			"s_bar_half_focus_r"
#define IMG_OPT_HALF_UNFOCUS		        "s_bar_half_unfocus"
#define IMG_OPT_HALF_FOCUS_EDIT_R		"s_bar_half_focus_r"

#define BUF_LEN	32
#define NUM_3G_ITEM	8
#define USB3G_DEV_HEAD	"USB3G"
#define BOX_3G_SET "box_3g_set_para"
#define TXT_3G_STATUS "text_3g_status_value"
#define IMG_3G_POP_BG "img_3g_pop_bg"
#define TXT_3G_POP_INFO "text_3g_pop_info"
#define TXT_3G_POP_TIP  "text_3g_pop_tip"
#define _3G_PASS_DISPLAY_STR "******"
enum
{
	_3G_TYPE_AUTO,
	_3G_TYPE_MANUAL,
};

enum
{
    _3G_PARAM_CHANGED,
    _3G_PARAM_NO_CHANGED,
    _3G_PARAM_INVALID,
};

enum
{
    _3G_TYPE = 0,
	_3G_AREA,
	_3G_OPEA,
	_3G_APN,
	_3G_DIAL,
	_3G_USER,
	_3G_PASS,
    _3G_SAVE,
	_3G_TOTA,
};

typedef enum
{
	_3G_CMB_CONNECT_AREA,
	_3G_CMB_CONNECT_OPERATOR,
} _3G_CONNECT_TYPE;

typedef struct
{
	char APN[BUF_LEN];
	char phone[BUF_LEN];
	char username[BUF_LEN];
	char password[BUF_LEN];
	char area[BUF_LEN];
	char operator[BUF_LEN];
}App3gPara;

typedef struct
{
	App3gPara	app3g;
}NetworkParam;

static int s_sel = 0;
static int curOperator = 1;
static event_list *s_3g_timer = NULL;
static ubyte32 s_cur_pass = {0};
static bool s_show_tip_ok = false;

static char *s_3g_opt[_3G_TOTA] = {
    "cmb_3g_set_type",
	"cmb_3g_set_area",
	"cmb_3g_set_operator",
	"btn_3g_set_apn",
	"btn_3g_set_phone",
	"btn_3g_set_username",
	"btn_3g_set_passwd",
	"btn_3g_set_save",
};

static char *s_3g_boxitem[_3G_TOTA] = {
    "boxitem_3g_set_type",
	"boxitem_3g_set_area",
	"boxitem_3g_set_operator",
	"boxitem_3g_set_apn",
	"boxitem_3g_set_phone",
	"boxitem_3g_set_username",
	"boxitem_3g_set_passwd",
	"boxitem_3g_set_save",
};
#define MAX_3GAREA_LEN           30
#define MAX_3GOPERATOR_LEN       40
#define MAX_3GAPN_LEN            30
#define MAX_3GAPNUSER_LEN        32
#define MAX_3GAPNPWD_LEN         32
#define MAX_3GDIALNUMBER_LEN     32

#define _3G_PRINTF(...) app_log_debug( __VA_ARGS__ )
#define DEBUG_3G_PRINT(p,n)\
do\
{\
	_3G_PRINTF("\nfunc:%s line:%d\t",__FUNCTION__,__LINE__);\
	_3G_PRINTF("zl (%s:%d)\n",p,n);\
}while(0)

typedef struct __3G_CONFIG_AREA
{
	unsigned short  AreaIndex;                	 /**< Area index */
	unsigned short  OperatorCount;               /**< count of aera*/
	unsigned char   AreaName[MAX_3GAREA_LEN]; 	 /**< Area Name */
} _3G_CONFIG_AREA;

typedef struct __3G_CONFIG_OPERATOR
{
	unsigned short  OperatorIndex;                    /**< operator index */
	unsigned short  AreaIndex;                        /**< Area index */
	unsigned char   UsedFlag;                         /**< bit 0: Inused status  bit 1: is used*/
	unsigned char   OperatorName[MAX_3GOPERATOR_LEN]; /**< operator name */
} _3G_CONFIG_OPERATOR;

typedef struct __3GApn_t
{
	unsigned char   APNName[MAX_3GAPN_LEN];           /**< APN name */
	unsigned char   DialNumber[MAX_3GDIALNUMBER_LEN]; /**< Dial Number*/
	unsigned char   APNUser[MAX_3GAPNUSER_LEN];       /**< APN User */
	unsigned char   APNPWD[MAX_3GAPNPWD_LEN];         /**< APN PWD */
} _3GApn_t;

typedef struct __3G_CONFIG_APN
{
	unsigned short  APNIndex;       		   /**< APN index */
	unsigned short  OperatorIndex;  		   /**< operator index */
	unsigned char   UsedFlag;       		   /**< bit 0: Inused status  bit 1: is used*/
	_3GApn_t        tApn;
} _3G_CONFIG_APN;

//const _3G_CONFIG_AREA g_3GDefaultAreaInfo[NUM_3G_CONFIG_AREA] = {
const _3G_CONFIG_AREA g_3GDefaultAreaInfo[] = {
	// AeraIndex OperatorCount AreaName
	{ 1, 3, "Egypt"},
	{ 2, 1, "Tunisia"},
	{ 3, 4, "Algeria"},
	{ 4, 1, "Morocco"},
	{ 5, 2, "United Arab Emirates"},
	{ 6, 4, "Iraq"},
	{ 7, 4, "Jordan"},
	{ 8, 1, "Kuwait"},
	{ 9, 2, "Brazil"},
	{ 10, 2,"Argentina"},
	{ 11, 3,"China"},
	{ 12, 1,"Taiwan"},
	{ 13, 2,"South Korea"},
	{ 14, 1, "India"},
};

//const _3G_CONFIG_OPERATOR g_3GDefaultOperatorInfo[NUM_3G_CONFIG_OPERATOR] = {
const _3G_CONFIG_OPERATOR g_3GDefaultOperatorInfo[] = {
	// operatorIndex  Areaindex UsedFlag operatorname
	{ 1, 1, 1,"MobiNiL"},
	{ 2, 1, 1,"Vodafone Egypt"},//wo
	{ 3, 1, 1,"Etisalat Egypt"},//wo
	{ 4, 2, 1,"Orange Tunisie"},//wo
	{ 5, 3, 1,"Wataniya Algeria"},//wo
	{ 6, 3, 1,"Djezzy"},//wo
	{ 7, 4, 1,"Medi Telecom"},
	{ 8, 5, 1,"Etisalat UAE"},//wo
	{ 9, 5, 1,"du"},//wo
	{ 10, 6, 1,"Asiacell"},//wo
	{ 11, 6, 1,"Korek Telecom"},//wo
	{ 12, 7, 1,"Zain"},//wo
	{ 13, 7, 1,"MOBILECOM"},//wo
	{ 14, 8, 1,"zain KW (MTC-Vodafone Kuwait)"},//wo
	{ 15, 9, 1,"Brasil Telecom Celular2 (Oi Brazil)"},
	{ 16, 9, 1,"Claro Brasil"},
	{ 17, 10, 1,"Telecom Personal"},
	{ 18, 10, 1,"Movistar Argentina "},
	{ 19, 11, 1,"unicom"},
	{ 20, 12, 1,"Taiwan Cellular"},
	{ 21, 13, 1,"SKT"},
	{ 22, 13, 1,"KT"},
	{ 23, 11, 1,"china net"},
	{ 24, 7, 1,"Umniah"},
	{ 25, 7, 1,"Orange"},
	{ 26, 3, 1,"Mobilis Web"},//Algeria
	{ 27, 3, 1,"Nedjma.dz"},
	{ 28, 6, 1,"Zain"},
	{ 29, 6, 1,"MTN Irancell"},
    { 30, 11, 1, "CMCC"},
	{ 31, 14, 1, "AIRTEL"},
};

_3G_CONFIG_APN g_3GDefaultAPNInfo[] = {
	//APNIndex  OperatorIndex UsedFlag APNName DialNumber APNUser APNPWD
	{ 1,  1,  1,{"mobinilweb",                 "*99#","",""}},
	{ 2,  2,  1, {"internet.vodafone.net",     "*99#","internet","internet"}},
    { 3,  3,  1,                                            },
    { 4,  4,  1,                                            },
    { 5,  5,  1,                                            },
    { 6,  6,  1,                                            },
	{ 7,  7,  1,{"wap.meditel.ma",             "*99#","",""}},
    { 8,  8,  1,                                            },
    { 9,  9,  1,                                            },
	{ 10, 10, 1,{"net. asiacell.com",          "*99#","",""}},
	{ 11, 11, 1,{"internet.korek.com",         "*99#","korek","korek"}},
    { 12, 12, 1,                                            },
    { 13, 13, 1,                                            },
    { 14, 14, 1,                                            },
	{ 15, 15, 1,{"gprs.oi.com.br",             "*99#","",""}},
	{ 16, 16, 1,{"claro.com.br",               "*99#","",""}},
	{ 17, 17, 1,{"gprs.personal.com",          "*99#","",""}},
	{ 18, 18, 1,{"internet.gprs.unifon.com.ar","*99#","",""}},
	{ 19, 19, 1,{"3gnet",                      "*99#","",""}},
	{ 20, 20, 1,{"internet",                   "*99#","",""}},
	{ 21, 21, 1,{"web.sktelecom.com",          "*99#","",""}},
	{ 22, 22, 1,{"alwaysson.ktfwing.com",      "*99#","",""}},
	{ 23, 23, 1,{"ctnet",                      "#777","ctnet@mycdma.cn","vnet.mobi"}},
	{ 24, 24, 1,{"umniah",                     "*99#","",""}},
	{ 25, 25, 1,{"net.orange.jo",              "*99***1#","net","net"}},
	{ 26, 26, 1,{"internet",                   "*99#","internet","internet"}},
	{ 27, 27, 1,{"internet",                   "*99#","nedjma","nedjma"}},
	{ 28, 28, 1,{"internet",                   "*99#","",""}},
	{ 29, 29, 1,{"mtnirancell",                "*99#","",""}},
    { 30, 30, 1,{"cmnet",                      "*99***1#", "",""}},
	{ 31, 31, 1,{"airtelgprs.com",             "*99***1#","",""}},
};

extern int app_pop_list_end_dialog(void);
static int _3g_check_param_change(void);
static void _3g_cmb_init(int area_cmb_sel, int operator_cmb_sel,_3G_CONNECT_TYPE type);
static void _3g_edit_apn(int area_cmb_sel,int operator_cmb_sel);
static void _init_mode_status(void);
static void _3g_dev_update(void);
static bool _3g_update(void);
static void _3g_type_change(void);
extern int app_network_3g_stop(void);

static ubyte64 cur3gDev = {0};


static void _hide_password(const char *password)
{
    int len_pass = 0;

    if(NULL == password)
    {
            return;
    }

    len_pass = strlen(password) > sizeof(ubyte32) - 1? sizeof(ubyte32) - 1: strlen(password);

    memset(s_cur_pass, 0, sizeof(ubyte32));
    memcpy(s_cur_pass, password, len_pass);

    if(len_pass>0)
    {
        GUI_SetProperty(s_3g_opt[_3G_PASS], "string", _3G_PASS_DISPLAY_STR);
    }
    else
    {
        GUI_SetProperty(s_3g_opt[_3G_PASS], "string", NULL);
    }
    return;
}

static void _3g_loop_setparam(ubyte64 dev_name)
{
    static int oper_id = 0;
    int type_cmb_sel = -1;
    GxMsgProperty_3G    tgParam;
    memset(&tgParam, 0, sizeof(tgParam));
    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
    if(_3G_TYPE_MANUAL != type_cmb_sel)
    {
        int count_apn = 0;
        count_apn = sizeof(g_3GDefaultAPNInfo) / sizeof(_3G_CONFIG_APN);
        strncpy(tgParam.dev_name, dev_name, strlen(dev_name));
        strcpy(tgParam.apn,(const char*)g_3GDefaultAPNInfo[oper_id].tApn.APNName);
        strcpy(tgParam.phone,(const char*)g_3GDefaultAPNInfo[oper_id].tApn.DialNumber);
        strcpy(tgParam.username,(const char*)g_3GDefaultAPNInfo[oper_id].tApn.APNUser);
        strcpy(tgParam.password,(const char*)g_3GDefaultAPNInfo[oper_id].tApn.APNPWD);
        tgParam.operator_id = g_3GDefaultAPNInfo[oper_id].OperatorIndex;
        app_send_msg_exec(GXMSG_3G_SET_PARAM, &tgParam);
        oper_id++;
        if(oper_id == count_apn)
        {
            oper_id = 0;
        }
    }
}

static void _3g_param_send(int type)
{
	GxMsgProperty_3G	tgParam;
	char *p[4] = {NULL};
	uint32_t len[4] = {0};
    int i = 0;
    int j = 0;

    memset(&tgParam, 0, sizeof(tgParam));

    if(_3G_TYPE_MANUAL == type)
    {
    for(i = _3G_APN; i < _3G_PASS; i++)
    {
        GUI_GetProperty(s_3g_opt[i], "string", &p[j]);
        len[j] = strlen(p[j])>sizeof(ubyte32)-1? sizeof(ubyte32)-1:strlen(p[j]);
        j++;
    }
    p[3] = s_cur_pass;
    len[3] = strlen(p[3]) > sizeof(ubyte32) - 1? sizeof(ubyte32) - 1:strlen(p[3]);

    tgParam.operator_id = curOperator;
    strncpy(tgParam.apn, p[0], len[0]);
    strncpy(tgParam.phone, p[1], len[1]);
    strncpy(tgParam.username, p[2], len[2]);
    strncpy(tgParam.password, p[3], len[3]);
    strncpy(tgParam.dev_name, cur3gDev, strlen(cur3gDev));
    }
	else
	{
        tgParam.operator_id = curOperator;
		strncpy(tgParam.dev_name, cur3gDev, strlen(cur3gDev));
	}
    app_send_msg_exec(GXMSG_3G_SET_PARAM, &tgParam);

    return;
}

static int _3g_check_param_change(void)
{
	char *p[4] = {NULL};
	GxMsgProperty_3G	tgParam;
    GxMsgProperty_3G_Operator operator;
    int cur_type_sel = -1;
    int type_cmb_sel = -1;

    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
    memset(&tgParam, 0, sizeof(GxMsgProperty_3G));
    strncpy(tgParam.dev_name, cur3gDev, strlen(cur3gDev));
    tgParam.operator_id = curOperator;
    app_send_msg_exec(GXMSG_3G_GET_PARAM, &tgParam);
    app_send_msg_exec(GXMSG_3G_GET_CUR_OPERATOR, &operator);

	GUI_GetProperty(s_3g_opt[_3G_TYPE], "select", &cur_type_sel);
	GUI_GetProperty(s_3g_opt[_3G_APN], "string", &p[0]);
	GUI_GetProperty(s_3g_opt[_3G_DIAL], "string", &p[1]);
	GUI_GetProperty(s_3g_opt[_3G_USER], "string", &p[2]);
    p[3] = s_cur_pass;

    if((cur_type_sel == _3G_TYPE_MANUAL)&&(0 == strlen(p[0]) || 0 == strlen(p[1])))
    {
        return _3G_PARAM_INVALID;
    }
	else if ((strcmp(tgParam.apn, p[0]) != 0)
	    	||(strcmp(tgParam.phone, p[1]) != 0)
		    ||(strcmp(tgParam.username, p[2]) != 0)
		    ||(strcmp(tgParam.password, p[3]) != 0)
			||(cur_type_sel != type_cmb_sel)
            ||(operator.operator_id != curOperator))
	{
		return _3G_PARAM_CHANGED;
	}

	return _3G_PARAM_NO_CHANGED;
}

static void _3g_save_change(void)
{
    int type_cmb_sel = -1;
    int area_cmb_sel = -1;
    int operator_cmb_sel = -1;
    int cur_type_sel = -1;
    int cur_area_sel = -1;
    int cur_operator_sel = -1;
	GxMsgProperty_NetDevState state;

	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strncpy(state.dev_name, cur3gDev, strlen(cur3gDev));
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);

	GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
    GxBus_ConfigGetInt(_3G_AREA_KEY, &area_cmb_sel, _3G_AREA_DEFAULT);
    GxBus_ConfigGetInt(_3G_OPEA_SEL_KEY, &operator_cmb_sel, _3G_OPEA_SEL_DEFAULT);
    GUI_GetProperty(s_3g_opt[_3G_TYPE], "select", &cur_type_sel);
    GUI_GetProperty(s_3g_opt[_3G_AREA], "select", &cur_area_sel);
    GUI_GetProperty(s_3g_opt[_3G_OPEA], "select", &cur_operator_sel);

    if(area_cmb_sel != cur_area_sel || operator_cmb_sel != cur_operator_sel)
    {
        GxBus_ConfigSetInt(_3G_AREA_KEY, cur_area_sel);
        GxBus_ConfigSetInt(_3G_OPEA_SEL_KEY, cur_operator_sel);
    }
    if(type_cmb_sel != cur_type_sel)
    {
        GxBus_ConfigSetInt(_3G_TYPE_KEY, cur_type_sel);

        if(_3G_TYPE_AUTO == cur_type_sel)
        {
           _3g_param_send(_3G_TYPE_AUTO);
        }
        else
        {
			if(_3G_PARAM_INVALID == _3g_check_param_change())
				return;

            _3g_param_send(_3G_TYPE_MANUAL);
        }
    }
    else
    {
        if(_3G_TYPE_AUTO == type_cmb_sel)
        {
			if(state.dev_state != IF_STATE_CONNECTED)
			{
				_3g_param_send(_3G_TYPE_AUTO);
			}
            return;
        }
        if(_3G_PARAM_CHANGED == _3g_check_param_change())
        {
            _3g_param_send(_3G_TYPE_MANUAL);
        }
    }

    return;
}

static void _3g_cmb_init(int area_cmb_sel, int operator_cmb_sel, _3G_CONNECT_TYPE type)
{
    int i = 0;
    int count = 0;
    char *p = NULL;
    if(_3G_CMB_CONNECT_AREA == type)
    {
        count = sizeof(g_3GDefaultAreaInfo) / sizeof(_3G_CONFIG_AREA);
        p = GxCore_Calloc(count * MAX_3GAREA_LEN + (count - 1) + 2, 1); //'count - 1' is ',' '+2' for '[' and ']'
        if(NULL == p)
        {
            while(1)
            {
                app_log_error("Malloc Error %s--%d\n", __func__, __LINE__);
            }
        }

        strcat(p, "[");
        for(i = 0; i < count; i++)
        {
            strcat(p, (char *)g_3GDefaultAreaInfo[i].AreaName);
            if(i == count - 1)
            {
                break;
            }
            strcat(p, ",");
        }
        strcat(p, "]");

        GUI_SetProperty(s_3g_opt[_3G_AREA], "content", p);
        GUI_SetProperty(s_3g_opt[_3G_AREA], "select", &area_cmb_sel);
        GxCore_Free(p);
        p = NULL;
    }
    else if(_3G_CMB_CONNECT_OPERATOR == type)
    {
        int count_operator = 0;

        count_operator = sizeof(g_3GDefaultOperatorInfo) / sizeof(_3G_CONFIG_OPERATOR);

        for(i = 0; i < count_operator; i++)
        {
            if((area_cmb_sel + 1) == g_3GDefaultOperatorInfo[i].AreaIndex)
            {
                count++;
            }
        }
        p = GxCore_Calloc(count * MAX_3GOPERATOR_LEN + (count - 1) + 2, 1);
        if(NULL == p)
        {
            while(1)
            {
                app_log_error("Malloc Error %s--%d\n", __func__, __LINE__);
            }
        }
        strcat(p, "[");
        for(i = 0; i < count_operator; i++)
        {
            if(area_cmb_sel + 1 == g_3GDefaultOperatorInfo[i].AreaIndex)
            {
                strcat(p, (char *)g_3GDefaultOperatorInfo[i].OperatorName);
                if(1 == count)
                {
                    break;
                }
                strcat(p, ",");
                count--;
            }
        }
        strcat(p, "]");

        GUI_SetProperty(s_3g_opt[_3G_OPEA], "content", p);
        GUI_SetProperty(s_3g_opt[_3G_OPEA], "select", &operator_cmb_sel);

        GxCore_Free(p);
        p = NULL;
    }

    return;
}

static void _3g_edit_apn(int area_cmb_sel, int operator_cmb_sel)
{
    int i = 0;
    int operator_sel = 0;
    int count_operator = 0;
    GxMsgProperty_3G tgParam;

    count_operator = sizeof(g_3GDefaultOperatorInfo) / sizeof(_3G_CONFIG_OPERATOR);
    GUI_SetProperty(s_3g_opt[_3G_APN], "clear", NULL);
    GUI_SetProperty(s_3g_opt[_3G_DIAL], "clear", NULL);
    GUI_SetProperty(s_3g_opt[_3G_USER], "clear", NULL);
    GUI_SetProperty(s_3g_opt[_3G_PASS], "clear", NULL);

    for(i = 0; i < count_operator; i++)
    {
        if(area_cmb_sel + 1 == g_3GDefaultOperatorInfo[i].AreaIndex)
        {
            if(operator_sel == operator_cmb_sel)
            {
                int count_apn = 0;
                int j = 0;
                count_apn = sizeof(g_3GDefaultAPNInfo) / sizeof(_3G_CONFIG_APN);

                for(j = 0; j < count_apn; j++)
                {
                    if(g_3GDefaultAPNInfo[j].OperatorIndex == g_3GDefaultOperatorInfo[i].OperatorIndex)
                    {
                        memset(&tgParam, 0, sizeof(GxMsgProperty_3G));
                        tgParam.operator_id = g_3GDefaultAPNInfo[j].OperatorIndex;
                        strncpy(tgParam.dev_name, cur3gDev, strlen(cur3gDev));
                        app_send_msg_exec(GXMSG_3G_GET_PARAM, &tgParam);
                        curOperator = g_3GDefaultAPNInfo[j].OperatorIndex;

                        if(0 == strlen(tgParam.apn) && 0 == strlen(tgParam.phone) &&
                                0 == strlen(tgParam.username) && 0 == strlen(tgParam.password))
                        {
                            GUI_SetProperty(s_3g_opt[_3G_APN], "string", (void *)g_3GDefaultAPNInfo[j].tApn.APNName);
                            GUI_SetProperty(s_3g_opt[_3G_DIAL], "string", (void *)g_3GDefaultAPNInfo[j].tApn.DialNumber);
                            GUI_SetProperty(s_3g_opt[_3G_USER], "string", (void *)g_3GDefaultAPNInfo[j].tApn.APNUser);
                            _hide_password((char *)g_3GDefaultAPNInfo[j].tApn.APNPWD);
                        }
                        else
                        {
                            GUI_SetProperty(s_3g_opt[_3G_APN], "string", (void *)tgParam.apn);
                            GUI_SetProperty(s_3g_opt[_3G_DIAL], "string", (void *)tgParam.phone);
                            GUI_SetProperty(s_3g_opt[_3G_USER], "string", (void *)tgParam.username);
                            _hide_password((char *)tgParam.password);
                        }
                        return;
                    }
                }
            }
            operator_sel++;
        }

    }

    return;
}


static void _init_mode_status(void)
{
    GxMsgProperty_NetDevMode  dev_mode;
    int type_cmb_sel = -1;
    int area_cmb_sel = -1;
    int operator_cmb_sel = -1;

    s_sel = 0;
    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    memcpy(dev_mode.dev_name, cur3gDev, strlen(cur3gDev));
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);

    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
    GxBus_ConfigGetInt(_3G_AREA_KEY, &area_cmb_sel, _3G_AREA_DEFAULT);
    GxBus_ConfigGetInt(_3G_OPEA_SEL_KEY, &operator_cmb_sel, _3G_OPEA_SEL_DEFAULT);
    GUI_SetProperty(s_3g_opt[_3G_TYPE], "select", &type_cmb_sel);

    _3g_cmb_init(area_cmb_sel, operator_cmb_sel, _3G_CMB_CONNECT_AREA);
    _3g_cmb_init(area_cmb_sel, operator_cmb_sel, _3G_CMB_CONNECT_OPERATOR);
    _3g_edit_apn(area_cmb_sel, operator_cmb_sel);
    _3g_type_change();

    return;
}

static char* _3g_get_state(IfState state)
{
    char *str = NULL;

    switch(state)
    {
        case IF_STATE_INVALID:
            str = STR_ID_INVALID;
            break;

        case IF_STATE_IDLE:
        case IF_STATE_REJ:
            str = STR_ID_NO_CONNECT;
            break;

        case IF_STATE_START:
        case IF_STATE_CONNECTING:
            str = STR_ID_CONNECTTING;
            break;

        case IF_STATE_CONNECTED:
            str = STR_ID_CONNECTED;
            break;

        default:
            break;
    }

    return str;
}

static void _3g_type_change(void)
{
    int type_cmb_sel = -1;
    int i = 0;

    GUI_GetProperty(s_3g_opt[_3G_TYPE], "select", &type_cmb_sel);
    if(_3G_TYPE_AUTO == type_cmb_sel)
     {
        for(i = _3G_AREA; i < _3G_SAVE; i++)
        {
            GUI_SetProperty(s_3g_boxitem[i], "disable", NULL);
        }
    }
    else if(_3G_TYPE_MANUAL == type_cmb_sel)
    {
        for(i = _3G_AREA; i < _3G_SAVE; i++)
        {
            GUI_SetProperty(s_3g_boxitem[i], "enable", NULL);
        }
    }

    return;
}
static void _3g_pop_info_hide(void)
{
    GUI_SetProperty(IMG_3G_POP_BG, "state", "hide");
    GUI_SetProperty(TXT_3G_POP_INFO, "state", "hide");
    GUI_SetProperty(TXT_3G_POP_TIP, "state", "hide");
    GUI_SetProperty(WND_3G,"update_all",NULL);
    s_show_tip_ok = false;
}

static int _3g_pop_info_timer_cb(void *use)
{
    if(NULL != s_3g_timer)
    {
        s_3g_timer = NULL;
    }

    _3g_pop_info_hide();
    _3g_update();
	if(_3G_PARAM_INVALID == _3g_check_param_change())
		return 0;

	_init_mode_status();
    _3g_dev_update();

    return 0;
}

static void _3g_pop_info_show(char *info_str, int time_out)
{
    s_show_tip_ok = true;
    GUI_SetProperty(IMG_3G_POP_BG, "state", "show");
    GUI_SetProperty(TXT_3G_POP_INFO, "state", "show");
    GUI_SetProperty(TXT_3G_POP_TIP, "state", "show");
    GUI_SetProperty(TXT_3G_POP_INFO, "string", (void *)info_str);
    if(time_out>0)
    {
        if(0 != reset_timer(s_3g_timer))
        {
            s_3g_timer = create_timer(_3g_pop_info_timer_cb, time_out, NULL, TIMER_ONCE);
        }
    }
}

static void _3g_dev_update(void)
{
	GxMsgProperty_NetDevState state;
    char *state_str = NULL;
    int type_cmb_sel = -1;
    int i = 0;

	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strncpy(state.dev_name, cur3gDev, strlen(cur3gDev));
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    state_str = _3g_get_state(state.dev_state);
    GUI_SetProperty(TXT_3G_STATUS, "string", state_str);
    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
	GUI_SetProperty(s_3g_opt[_3G_TYPE], "select", &type_cmb_sel);

	if(_3G_TYPE_MANUAL == type_cmb_sel)
	{
		for(i = _3G_AREA; i < _3G_SAVE; i++)
		{
			GUI_SetProperty(s_3g_boxitem[i], "enable", NULL);
		}
	}
	else
	{
		for(i = _3G_AREA; i < _3G_SAVE; i++)
		{
			GUI_SetProperty(s_3g_boxitem[i], "disable", NULL);
		}
	}

    return;
}

static bool _3g_update(void)
{
	uint8_t i = 0;
	GxMsgProperty_NetDevList dev_list;

    memset(cur3gDev, 0, sizeof(cur3gDev));
	memset(&dev_list, 0, sizeof(GxMsgProperty_NetDevList));
	app_send_msg_exec(GXMSG_NETWORK_DEV_LIST, &dev_list);

	if(0 == dev_list.dev_num)
	{
        return false;
	}
	else
	{
		for(i = 0; i < dev_list.dev_num; i++)
		{
			if(strncmp(dev_list.dev_name[i], USB3G_DEV_HEAD,strlen(USB3G_DEV_HEAD)) == 0)
			{
				if(strncmp(dev_list.dev_name[i], USB3G_DEV_HEAD,strlen(USB3G_DEV_HEAD)) == 0)
				{
					strcpy(cur3gDev, dev_list.dev_name[i]);
                    return true;
				}
			}
		}
	}

	if(dev_list.dev_num == i)
	{
        return false;
	}

    return true;
}



static void _3g_full_keyboard_proc(PopKeyboard *data)
{
    GUI_SetFocusWidget(BOX_3G_SET);
    GUI_SetProperty(BOX_3G_SET, "select", &s_sel);

	if(data->in_ret == POP_VAL_CANCEL)
		return;
	if (data->out_name == NULL)
		return;

    if(_3G_PASS == s_sel)
    {
        _hide_password(data->out_name);
    }
    else
    {
	    if(trim(data->out_name) == NULL)
          return;
        GUI_SetProperty(s_3g_opt[s_sel], "string", data->out_name);
    }

    GUI_SetInterface("flush", NULL);

    return;
}

/*type change */
SIGNAL_HANDLER int On_3g_set_cmb_type_change(GuiWidget *widget, void *usrdata)
{
    _3g_type_change();

    return EVENT_TRANSFER_KEEPON;
}

/* area change */
SIGNAL_HANDLER int On_3g_set_cmb_area_change(GuiWidget *widget, void *usrdata)
{
	uint32_t area_cmb_sel = 0;
	uint32_t operator_cmb_sel = 0;

	GUI_GetProperty(s_3g_opt[_3G_AREA], "select", &area_cmb_sel);
	_3g_cmb_init(area_cmb_sel, operator_cmb_sel,_3G_CMB_CONNECT_OPERATOR);
	_3g_edit_apn(area_cmb_sel,operator_cmb_sel);

	return EVENT_TRANSFER_KEEPON;
}

/* operator change */
SIGNAL_HANDLER int On_3g_set_cmb_operator_change(GuiWidget *widget, void *usrdata)
{
	uint32_t area_cmb_sel = 0;
	uint32_t operator_cmb_sel = 0;

	GUI_GetProperty(s_3g_opt[_3G_AREA], "select", &area_cmb_sel);
	GUI_GetProperty(s_3g_opt[_3G_OPEA], "select", &operator_cmb_sel);
    if(g_3GDefaultAreaInfo[area_cmb_sel].OperatorCount > 1)
    {
	    _3g_edit_apn(area_cmb_sel,operator_cmb_sel);
    }
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_btn_3g_set_apn_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_OK:
				{
					PopKeyboard keyboard;
					char *p = NULL;

					GUI_GetProperty(s_3g_opt[_3G_APN], "string", &p);
					app_log_debug("\nfunc:%s , p:%s\n",__FUNCTION__,p);

					s_sel = _3G_APN;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name  = p;//should get current apn;
					keyboard.max_num = BUF_LEN-1;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = _3g_full_keyboard_proc;
					keyboard.usr_data   = NETWORK_3G_OPTION_INVALID;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
					return EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_MENU:
			case STBK_EXIT:

			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_btn_3g_set_phone_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_OK:
				{
					PopKeyboard keyboard;
					char *p = NULL;

					GUI_GetProperty(s_3g_opt[_3G_DIAL], "string", &p);
					app_log_debug("\nfunc:%s , p:%s\n",__FUNCTION__,p);

					s_sel = _3G_DIAL;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name    = p;//should get current phone;
					keyboard.max_num = BUF_LEN-1;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = _3g_full_keyboard_proc;
					keyboard.usr_data   = NETWORK_3G_OPTION_INVALID;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
					return EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_MENU:
			case STBK_EXIT:

			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_btn_3g_set_username_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_OK:
				{
					PopKeyboard keyboard;
					char *p = NULL;

					GUI_GetProperty(s_3g_opt[_3G_USER], "string", &p);
					app_log_debug("\nfunc:%s , p:%s\n",__FUNCTION__,p);

					s_sel = _3G_USER;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name    = p;//should get current username;
					keyboard.max_num = BUF_LEN-1;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = _3g_full_keyboard_proc;
					keyboard.usr_data   = NETWORK_3G_OPTION_INVALID;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
					return EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_MENU:
			case STBK_EXIT:

			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}


SIGNAL_HANDLER int On_btn_3g_set_passwd_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_OK:
				{
					PopKeyboard keyboard;
					char *p = NULL;

                    p = s_cur_pass;
					app_log_debug("\nfunc:%s , p:%s\n",__FUNCTION__,p);

					s_sel = _3G_PASS;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name    = p;//should get current passwd;
					keyboard.max_num = BUF_LEN-1;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = _3g_full_keyboard_proc;
					keyboard.usr_data   = NETWORK_3G_OPTION_INVALID;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
					return EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_MENU:
			case STBK_EXIT:

			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_box_3g_set_para_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	uint32_t box_sel;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{

				case STBK_UP:
					GUI_GetProperty(BOX_3G_SET, "select", &box_sel);
					if(_3G_TYPE == box_sel)
					{
						box_sel = _3G_SAVE;
						GUI_SetProperty(BOX_3G_SET, "select", &box_sel);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}

					break;

				case STBK_DOWN:
					GUI_GetProperty(BOX_3G_SET, "select", &box_sel);
					if(_3G_SAVE == box_sel)
					{
						box_sel = _3G_TYPE;
						GUI_SetProperty(BOX_3G_SET, "select", &box_sel);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
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

SIGNAL_HANDLER int On_3g_btn_ok_keypress(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NetDevState state;
    GxMsgProperty_3G_Operator operator;
    GxMsgProperty_3G tgParam;
    GUI_Event *event = NULL;

	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strncpy(state.dev_name, cur3gDev, strlen(cur3gDev));
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode, event->key.sym))
        {
            case STBK_OK:
                if(false == s_show_tip_ok)
                {
                    int ret;
                    ret = _3g_check_param_change();
                    if(_3G_PARAM_INVALID == ret)
                    {
                        _3g_pop_info_show(STR_ID_INVALID, 1000);
                        return EVENT_TRANSFER_STOP;
                    }
                    else if(_3G_PARAM_NO_CHANGED == ret
                            && (IF_STATE_IDLE == state.dev_state || IF_STATE_REJ == state.dev_state)
                            )
                    {
                        memset(&tgParam, 0, sizeof(GxMsgProperty_3G));
                        app_send_msg_exec(GXMSG_3G_GET_CUR_OPERATOR, &operator);
                        strncpy(tgParam.dev_name, cur3gDev,strlen(cur3gDev));
                        tgParam.operator_id = operator.operator_id;
                        app_send_msg_exec(GXMSG_3G_GET_PARAM, &tgParam);
                        app_send_msg_exec(GXMSG_3G_SET_PARAM, &tgParam);
                    }
                    else if(_3G_PARAM_CHANGED == ret)
                    {
                        _3g_save_change();
                    }

                    _3g_pop_info_show(STR_ID_WAITING, 1000);
                }
                break;

            default:
                _3g_pop_info_hide();
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  On_3g_create(const char* widgetname, void *usrdata)
{
    _3g_dev_update();
    _init_mode_status();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_3g_destroy(const char* widgetname, void *usrdata)
{
    if(NULL != s_3g_timer)
    {
        remove_timer(s_3g_timer);
        s_3g_timer = NULL;
    }
    s_show_tip_ok = false;

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int On_3g_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_MENU:
			case STBK_EXIT:
			    GUI_EndDialog(WND_3G);
				break;

			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_3g_got_focus(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_3g_lost_focus(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void _3g_state_msg_proc(GxMsgProperty_NetDevState *state)
{
    static IfState s_3gstate = IF_STATE_REJ;
    char *state_str = NULL;

    if(state == NULL)
    {
        return;
    }

    if(strncmp(state->dev_name, USB3G_DEV_HEAD,strlen(USB3G_DEV_HEAD)) != 0)// TODO:need to add 3g device
	{
		return;
	}
    if(state->dev_state == IF_STATE_IDLE && s_3gstate == IF_STATE_CONNECTING)
    {
        _3g_loop_setparam(state->dev_name);
    }
    s_3gstate = state->dev_state;

    state_str = _3g_get_state(state->dev_state);
    GUI_SetProperty(TXT_3G_STATUS, "string", state_str);
    if((!GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
        &&(!strcmp(GUI_GetFocusWindow(),WND_POP_OPTION_LIST)))
    {
        GUI_SetProperty(WND_POP_OPTION_LIST, "update", NULL);
    }
    return;
}


void app_3g_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
        {
            GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
	        app_log_debug("\nNET SETTING, %s, %s, %d\n",__FILE__,__FUNCTION__,__LINE__);
            _3g_state_msg_proc(dev_state);
            break;
        }

        case GXMSG_NETWORK_PLUG_OUT:
        {
            ubyte64 *dev_3g = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            GxMsgProperty_NetDevType type;

            memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
            strlcpy(type.dev_name, *dev_3g, sizeof(type.dev_name));
            app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);

            if(IF_TYPE_3G == type.dev_type)
            {
		#ifdef ECOS_OS
		//	app_network_3g_stop();
		#endif
                if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
                {
                    app_pop_list_end_dialog();
                }
                if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
                {
                    popdlg_destroy();
                }
                if(0 == strcmp(WND_3G, GUI_GetFocusWindow()))
                {
                    GUI_EndDialog(WND_3G);
                }
                else
                {
                    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
                    {
                        GUI_EndDialog("after wnd_network");
                        book_popdlg_recreate();
                    }
                    else
                    {
                        GUI_EndDialog("after wnd_network");
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}


status_t app_create_3g_menu(ubyte64 *dev_3g)
{
    if(dev_3g == NULL)
    {
        return GXCORE_ERROR;
    }

    if(false == _3g_update())
    {
        return GXCORE_ERROR;
    }
    GUI_CreateDialog(WND_3G);

    return GXCORE_SUCCESS;
}

#endif //NETWORK_SUPPORT && _3G_SUPPORT
