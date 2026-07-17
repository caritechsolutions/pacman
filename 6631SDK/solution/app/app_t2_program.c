#include "app_default_params.h"
#include "module/channel_edit.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_send_msg.h"
#include "app_book.h"
#include "app_windows.h"

extern void app_main_menu_listview_update(void);
#if SMART_VOLUME_SUPPORT
extern void app_smart_volume_support(int32_t volume_scope);
#endif

typedef enum
{
    SORT_SERVICE_ID = 0,
    SORT_SERVICE_NAME,
#if LCN_SUPPORT
    SORT_LCN,
#endif
    SORT_TPP,//tp,just diff the chanel edit.h tp
    SORT_MAX,
}win_t2_usrlist_sort_flag;

typedef enum
{
    LCN_STATE_OFF = 0, /*LCNر*/
    LCN_STATE_ON,
}Lcn_State_t;

typedef enum
{
    ACU_STATE_OFF = 0, /*ACUر*/
    ACU_STATE_ON,
}Acu_State_t;

typedef enum
{
    VOLUM_MODE_GLOBAL = 0,
    VOLUM_MODE_LOCAL,
#if SMART_VOLUME_SUPPORT
    VOLUM_MODE_SMART,
#endif
    VOLUM_MODE_TOTAL
}win_t2_volum_mode;

static char *s_volum_scope_content[VOLUM_MODE_TOTAL] = {"Global", "Channel",
#if SMART_VOLUME_SUPPORT
    "Smart"
#endif
};

void app_t2_sort_program(int32_t sort_flag)
{
    uint32_t focus = 0;
    uint32_t type;
    GxBusPmViewInfo view_info;
    GxBusPmViewInfo view_info_bak;
    GxMsgProperty_NodeNumGet node_num_get = {0};

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info_bak = view_info;

    for (type = GXBUS_PM_PROG_TV; type < GXBUS_PM_PROG_RADIO + 1; type++)
    {
		focus = 0;
        view_info.stream_type = type;

        GxBus_PmViewInfoMemSet(&view_info);
        node_num_get.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
        if(node_num_get.node_num == 0)
            continue;

        g_AppChOps.create(node_num_get.node_num);
        if (SORT_SERVICE_NAME == sort_flag)
            g_AppChOps.sort(SORT_A_Z, &focus);
#if LCN_SUPPORT
        else if(SORT_LCN == sort_flag)
            g_AppChOps.sort(SORT_BY_LCN, &focus);
#endif
        else if (SORT_TPP == sort_flag)
            g_AppChOps.sort(SORT_TP, &focus);
        else if(SORT_SERVICE_ID == sort_flag)
            g_AppChOps.sort(SORT_BY_SERVICE_ID, &focus);

        g_AppChOps.save();

    }
    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
    if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&view_info_bak))
    {
        return ;
    }
}

int app_program_volum_scope_keypress(int key)
{
    int volume_scope = VOLUM_MODE_LOCAL;
#if SMART_VOLUME_SUPPORT
    int volume_modes[] = {VOLUM_MODE_GLOBAL, VOLUM_MODE_LOCAL, VOLUM_MODE_SMART};
    int num_modes = 3;
    int current_index = 0;
    int i = 0;
#endif
    GxBus_ConfigGetInt(VOLUME_MODE_KEY, &volume_scope, VOLUME_SCOMPE_);

    if(key != STBK_RIGHT && key != STBK_LEFT)
        return EVENT_TRANSFER_KEEPON;
#if SMART_VOLUME_SUPPORT
    // Find the current index
    for (i = 0; i < num_modes; i++) {
        if (volume_scope == volume_modes[i]) {
            current_index = i;
            break;
        }
    }

    // Update the index based on key press
    if (key == STBK_RIGHT) {
        current_index = (current_index + 1) % num_modes;
    } else if (key == STBK_LEFT) {
        current_index = (current_index - 1 + num_modes) % num_modes;
    }

    // Set the new volume scope
    volume_scope = volume_modes[current_index];

    app_smart_volume_support(volume_scope);
#else
    if(volume_scope == VOLUM_MODE_GLOBAL)
        volume_scope = VOLUM_MODE_LOCAL;
     else if(volume_scope == VOLUM_MODE_LOCAL)
        volume_scope = VOLUM_MODE_GLOBAL;
#endif
    GxBus_ConfigSetInt(VOLUME_MODE_KEY, volume_scope);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_program_volum_scope_content_get(void)
{
    int volume_scope = 0;

    GxBus_ConfigGetInt(VOLUME_MODE_KEY, &volume_scope, VOLUME_SCOMPE_);
    return s_volum_scope_content[volume_scope];
}

int app_program_sort_keypress(int key)
{
    int32_t sort_flag;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(T2_SORT_FLAG, &sort_flag, T2_SORT_FLAG_VALUE);
    if(key == STBK_LEFT)
    {
        if(sort_flag == 0)
            sort_flag = SORT_MAX - 1;
        else
            sort_flag--;

        GxBus_ConfigSetInt(T2_SORT_FLAG, sort_flag);
        app_main_menu_listview_update();
    }
    else if(key == STBK_RIGHT)
    {
        if(sort_flag == (SORT_MAX - 1))
            sort_flag = 0;
        else
            sort_flag++;

        GxBus_ConfigSetInt(T2_SORT_FLAG, sort_flag);
        app_main_menu_listview_update();
    }
    app_t2_sort_program(sort_flag);
    return EVENT_TRANSFER_KEEPON;
}

char *app_program_sort_content_get(void)
{
    int32_t sort_flag;
    char *string = NULL;

    GxBus_ConfigGetInt(T2_SORT_FLAG, &sort_flag, T2_SORT_FLAG_VALUE);

    switch(sort_flag)
    {
        case SORT_SERVICE_ID:
            string = STR_ID_SORT_BY_ID;
            break;
        case SORT_SERVICE_NAME:
            string = STR_ID_SORT_BY_NAME;
            break;
#if LCN_SUPPORT
        case SORT_LCN:
            string = STR_ID_SORT_BY_LCN;
            break;
#endif
        case SORT_TPP:
            string = STR_ID_SORT_BY_TP;
            break;
        default:
            break;
    }

    return string;
}
#if IPTV_LITE_SUPPORT
int app_poweron_play_keypress(int key)
{
    int32_t poweron_play;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(POWERON_PLAY_KEY, &poweron_play, POWERON_PLAY_VALUE);
    if(poweron_play == 0)
    {
        poweron_play = 1;
    }
    else
    {
        poweron_play = 0;
    }
    GxBus_ConfigSetInt(POWERON_PLAY_KEY, poweron_play);

    return EVENT_TRANSFER_KEEPON;
}

char *app_poweron_play_content_get(void)
{
    int32_t poweron_play;
    char *string = NULL;

    GxBus_ConfigGetInt(POWERON_PLAY_KEY, &poweron_play, POWERON_PLAY_VALUE);

    if(0 == poweron_play)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif
#if LCN_SUPPORT
int app_lcn_setting_keypress(int key)
{
    int32_t lcn_config;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == LCN_STATE_OFF)
        GxBus_ConfigSetInt(LCN_KEY, LCN_STATE_ON);
    else if(lcn_config == LCN_STATE_ON)
        GxBus_ConfigSetInt(LCN_KEY, LCN_STATE_OFF);

    return EVENT_TRANSFER_KEEPON;
}

char *app_lcn_setting_content_get(void)
{
    int32_t lcn_config;
    char *string = NULL;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);

    if(LCN_STATE_OFF == lcn_config)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif

#if AUTO_UPDATE_SUPPORT
int app_acu_setting_keypress(int key)
{
	int32_t acu_config;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

#if PROGRAM_NETUP_SUPPORT
    extern bool app_prog_netup_workmode_get(void);
    if(app_prog_netup_workmode_get() == true)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.default_ret = POP_VAL_CANCEL;
        pop.str = STR_ID_PROGRAM_NETUP_CLOSE;
        pop.timeout_sec = 3000;
        popdlg_create(&pop);
        return EVENT_TRANSFER_STOP;
    }
#endif

	GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);
	if (acu_config == ACU_STATE_OFF)
	{
	    GxBus_ConfigSetInt(T2_ACU_KEY, ACU_STATE_ON);
	}
	else if (acu_config == ACU_STATE_ON)
	{
	    GxBus_ConfigSetInt(T2_ACU_KEY, ACU_STATE_OFF);
	}

    return EVENT_TRANSFER_KEEPON;
}
char *app_acu_setting_content_get(void)
{
    int32_t acu_config;
    char *string = NULL;

    GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);

    if(ACU_STATE_OFF == acu_config)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}

#endif
