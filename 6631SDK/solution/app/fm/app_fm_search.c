#include "app.h"
#if FM_SUPPORT
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_fm.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#define FM_NUM_ONE_PAGE       6
#define FREQ_BUFFER_LENGTH    (20+FM_FREQ_NAME_LEN)
#define FM_STRENGTH_MIN       0 //If 0, enable the automatic calculation of signal strength value, otherwise it is the signal strength value

#define TEXT_FM_SEARCH_TITLE         "text_fm_search_title"
#define TEXT_FM_SEARCH_PERCENT       "text_fm_search_progress_percent"
#define PROGBAR_FM_SEARCH_PROGRESS   "progbar_fm_search_progress"
#define IMG_FM_SEARCH_GIF            "img_fm_search_gif"

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"

static event_list* sp_FmShowFreqTimer = NULL;
static event_list* sp_FmGifTimer = NULL;
static int s_fm_list_cur_page = 0;
static uint32_t* s_show_freq_array = NULL;
static int s_search_start_flag = 0;

static int app_fm_search_draw_gif(void* usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "draw_gif", &alu);
    return 0;
}

static void app_fm_search_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "load_img", GIF_PATH);
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "init_gif_alu_mode", &alu);
}

static void app_fm_search_free_gif(void)
{
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "load_img", NULL);
}

static void app_fm_search_show_gif(void)
{
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "state", "show");
    if(0 != reset_timer(sp_FmGifTimer))
    {
        sp_FmGifTimer = create_timer(app_fm_search_draw_gif, 100, NULL, TIMER_REPEAT);
    }
}

static void app_fm_search_hide_gif(void)
{
    APP_TIMER_REMOVE(sp_FmGifTimer);
    GUI_SetProperty(IMG_FM_SEARCH_GIF, "state", "hide");
}

static int _fm_search_exit(PopDlgRet ret)
{
    GUI_EndDialog(WND_FM_SEARCH);
    return 0;
}

static void app_fm_search_info_tip(char* str)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.create_cb = NULL;
    pop.exit_cb = _fm_search_exit;
    pop.timeout_sec = 2;
    popdlg_create(&pop);
}


static int _fm_freq_show_list_cb(void *userdata)
{
    char Buffer[FREQ_BUFFER_LENGTH] = {0};
    int freq_num = 0;
    int show_num = 0;
    int i = 0;
    char* s_TextName[FM_NUM_ONE_PAGE] = {"text_search_fm_name1", "text_search_fm_name2",
                                        "text_search_fm_name3", "text_search_fm_name4",
                                        "text_search_fm_name5", "text_search_fm_name6"};
    static int value = 0;
    popdlg_destroy();
    if(s_search_start_flag == 1)
    {
        value = value + 3;
        if(value > 70)
            value = 70;
        memset(Buffer, 0, FREQ_BUFFER_LENGTH);
        sprintf((char*)Buffer, "%d%%", value);
        GUI_SetProperty(TEXT_FM_SEARCH_PERCENT, "string", Buffer);
        GUI_SetProperty(PROGBAR_FM_SEARCH_PROGRESS, "value", &value);
    }
    else if(s_search_start_flag == 2)
    {
        freq_num = app_fm_freq_num_get();
        if(freq_num == 0)
        {
            value = 100;
            GUI_SetProperty(TEXT_FM_SEARCH_PERCENT, "string", "100%");
            GUI_SetProperty(PROGBAR_FM_SEARCH_PROGRESS, "value", &value);
            app_fm_search_info_tip(STR_ID_FM_NO_FIND);
            timer_stop(sp_FmShowFreqTimer);
            return 0 ;
        }
        if(s_show_freq_array == NULL)
        {
            s_show_freq_array=(uint32_t*)GxCore_Malloc(freq_num*sizeof(uint32_t));
            if(s_show_freq_array == NULL)
            {
                app_log_error("fm search show list error \n");
                timer_stop(sp_FmShowFreqTimer);
                GUI_EndDialog(WND_FM_SEARCH);
                return 0;
            }
            app_fm_freq_array_get(s_show_freq_array);

        }
        if(s_fm_list_cur_page*FM_NUM_ONE_PAGE >= freq_num)
        {
            value = 100;
            memset(Buffer, 0, FREQ_BUFFER_LENGTH);
            sprintf((char*)Buffer, "%d%%", value);
            GUI_SetProperty(TEXT_FM_SEARCH_PERCENT, "string", Buffer);
            GUI_SetProperty(PROGBAR_FM_SEARCH_PROGRESS, "value", &value);
            value = 0;
            GUI_EndDialog(WND_FM_SEARCH);
        }
        else
        {
            if((freq_num-s_fm_list_cur_page*FM_NUM_ONE_PAGE) >= FM_NUM_ONE_PAGE)
                show_num = FM_NUM_ONE_PAGE;
            else
                show_num = freq_num - s_fm_list_cur_page*FM_NUM_ONE_PAGE;
            for(i=0; i<FM_NUM_ONE_PAGE; i++)
            {
                int index = s_fm_list_cur_page*FM_NUM_ONE_PAGE + i;
                if(i>show_num-1)
                    GUI_SetProperty(s_TextName[i], "string", " ");
                else
                {
                    memset(Buffer, 0, FREQ_BUFFER_LENGTH);
#if FM_FREQ_NAME_SUPPORT
                    sprintf((char*)Buffer,"%d    %.1f MHz %s", index+1, s_show_freq_array[index]/1000.0,app_fm_get_freq_name(s_show_freq_array[index]));
#else
                    sprintf((char*)Buffer,"%d    %.1f MHz", index+1, s_show_freq_array[index]/1000.0);
#endif
                    GUI_SetProperty(s_TextName[i], "string", Buffer);
                }
                app_update_pop_dlg(WND_POP_BOOK);
            }
            value = 70 + 30*(show_num + s_fm_list_cur_page*FM_NUM_ONE_PAGE)/freq_num;
            if(value > 100)
                value = 100;
            memset(Buffer, 0, FREQ_BUFFER_LENGTH);
            sprintf((char*)Buffer, "%d%%", value);
            GUI_SetProperty(TEXT_FM_SEARCH_PERCENT, "string", Buffer);
            GUI_SetProperty(PROGBAR_FM_SEARCH_PROGRESS, "value", &value);
            s_fm_list_cur_page++;
        }
    }
    return 0;
}

static void _fm_start_search(void* arg)
{
    uint32_t start_freq = 0;
    uint32_t end_freq = 0;
    uint32_t step_freq = 0;
    uint32_t search_num = 0;
    int32_t value = 0;
    int32_t handle = -1;
    uint32_t* search_freq_array = NULL;
    GxFeFMGetScanParams param = {0};

    GxCore_ThreadDetach();

    handle = app_fm_get_handle();
    if(handle < 0)
        return;

    GxBus_ConfigGetInt(FM_STEP, &value, FM_STEP_VAL);
    step_freq = 100*value;
    app_fm_get_band_range(&start_freq, &end_freq);
    search_num = (end_freq - start_freq)/step_freq + 1;
    search_freq_array = (uint32_t*)GxCore_Malloc(search_num*sizeof(uint32_t));
    if(search_freq_array == NULL)
    {
        app_log_error("fm search malloc error \n");
        return;
    }

    memset(search_freq_array, 0, search_num * sizeof(uint32_t));
    s_search_start_flag = 1;
    app_fm_search_show_gif();
    param.start_freq_KHz = start_freq;
    param.end_freq_KHz = end_freq;
    param.freq = search_freq_array;
    param.strength_min = FM_STRENGTH_MIN;
    param.scan_step_KHz = step_freq;
    GxFeGetFMScan(handle, &param);
    app_fm_search_hide_gif();
    s_search_start_flag = 2;

    if(param.freq_count > 0)
    {
        app_fm_freq_save(search_freq_array, param.freq_count);
    }
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_prog_num_update();
#endif
    GXCORE_FREE(search_freq_array);

    return;
}

SIGNAL_HANDLER int app_fm_search_create(GuiWidget *widget, void *usrdata)
{
    int value = 0;
    char Buffer[20]={0};
    int thread_id = 0;
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_unset_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif
    app_fm_freq_del_file();
    app_fm_data_init();
    GUI_SetProperty(TEXT_FM_SEARCH_TITLE, "string", STR_ID_FM_AUTO_SEARCH);
    sprintf((char*)Buffer, "%d%%", value);
    GUI_SetProperty(TEXT_FM_SEARCH_PERCENT,"string",Buffer);
    GUI_SetProperty(PROGBAR_FM_SEARCH_PROGRESS, "value", &value);
    app_fm_search_load_gif();
    s_fm_list_cur_page = 0;

    GxBus_ConfigGetInt(FM_STEP, &value, FM_STEP_VAL);
    if(value == 0)
    {
        app_fm_search_info_tip(STR_ID_FM_SEARCH_ERROR);
        return EVENT_TRANSFER_STOP;
    }
    GxCore_ThreadCreate("fm_search", &thread_id, _fm_start_search,
                        NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);

    APP_TIMER_ADD(sp_FmShowFreqTimer, _fm_freq_show_list_cb, 1000, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_search_destroy(GuiWidget *widget, void *usrdata)
{
    s_search_start_flag = 0;
    s_fm_list_cur_page = 0;
    APP_TIMER_REMOVE(sp_FmShowFreqTimer);
    GXCORE_FREE(s_show_freq_array);
    app_fm_search_hide_gif();
    app_fm_search_free_gif();
#if FM_FREQ_NAME_SUPPORT
    fm_freq_name_list_destroy();
#endif
    if(padmux_check(34, 1) != 0)
        padmux_set(34, 1);
#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_search_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

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
                 case VK_BOOK_TRIGGER:
                    GxFeSetFMScanExit();
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                     GxFeSetFMScanExit();
                     GUI_EndDialog(WND_FM_SEARCH);
                     break;
            }
    }
    return EVENT_TRANSFER_STOP;
}

void app_fm_auto_search_exec(void)
{
#if DVB2IP_SERVER_SUPPORT
if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    if(padmux_check(34, 4) != 0)
        padmux_set(34, 4);

    GUI_CreateDialog(WND_FM_SEARCH);
}

#endif
