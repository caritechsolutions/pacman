#include "app_dvb2ip_platform.h"
#if DVB2IP_SERVER_SUPPORT
#include "app.h"
#include "full_screen.h"
#include "module/app_frontend.h"
#include "module/app_ioctl.h"
#include "module/app_play_control.h"
#include "app_windows.h"
#include "module/channel_edit.h"

#define DVB2IP_ERROR		app_log_error

static int _dvb2ip_set_tp(GxBusPmDataSat sat, GxBusPmDataTP tp, GxBusPmDataProg prog)
{
    AppFrontend_SetTp params = {0};

    if(0 == sat.id || 0 == tp.id || 0 == prog.id)
        return -1;

    params.fre = app_show_to_tp_freq(&sat, &tp);
    params.symb = tp.tp_s.symbol_rate;
    params.pls_n = tp.tp_s.pls_n;
    params.stream_size = tp.tp_s.stream_size;
    params.polar = app_show_to_lnb_polar(&sat);

    if(params.polar == SEC_VOLTAGE_ALL)
        params.polar = app_show_to_tp_polar(&tp);

    params.sat22k = app_show_to_sat_22k(sat.sat_s.switch_22K, &tp);
    params.tuner = sat.tuner;

#if UNICABLE_SUPPORT
extern int app_unicable_param_check(GxBusPmDataSat sat);
    uint32_t centre_frequency = 0;//实际中频
    uint32_t lnb_index = 0;
    uint32_t set_tp_req = 0; //用户下行频率
    if(app_unicable_param_check(sat))
    {
        lnb_index = app_calculate_set_freq_and_initfreq(&sat, &tp, &set_tp_req, &centre_frequency);
        params.fre = set_tp_req;
        params.lnb_index = lnb_index;
    }
    else
    {
        params.lnb_index = 0x20;
    }

    params.centre_fre = centre_frequency;
    params.if_channel_index = sat.sat_s.unicable_para.if_channel;
    params.if_fre = sat.sat_s.unicable_para.centre_fre[params.if_channel_index];
#else
    params.lnb_index = 0x20;
    params.centre_fre = 0;
    params.if_channel_index = 0;
    params.if_fre = 0;
#endif

#if MULTISTREAM_SUPPORT
    if(tp.sat_id == prog.sat_id && tp.id == prog.tp_id && 1 == prog.muti_ts_exist)
    {
        params.pls_ts_id = prog.muti_ts_id;
        params.pls_code = prog.muti_ts_code;
    }
#endif

    switch(sat.type)
    {
        case GXBUS_PM_SAT_S:
            {
                params.type = FRONTEND_DVB_S;
                break;
            }
        case GXBUS_PM_SAT_C:
            {
#if DEMOD_DVB_C
                params.fre = tp.frequency;
                params.symb = tp.tp_c.symbol_rate;
                params.qam = tp.tp_c.modulation;
#endif
                params.type = FRONTEND_DVB_C;
                break;

            }
        case GXBUS_PM_SAT_T:
            {
                params.type = FRONTEND_DVB_T;
                break;
            }
        case GXBUS_PM_SAT_DVBT2:
            {
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
                params.fre = tp.frequency;
                params.bandwidth = tp.tp_dvbt2.bandwidth;
                params.data_plp_id = prog.data_plp_id;
                params.common_plp_exist = prog.common_plp_exist;
                params.common_plp_id = prog.common_plp_id;
                params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif
                params.type = FRONTEND_DVB_T2;
                break;
            }
#if DEMOD_DTMB
        case GXBUS_PM_SAT_DTMB:
            if(GXBUS_PM_SAT_1501_DTMB == sat.sat_dtmb.work_mode)
            {
                params.fre = tp.frequency;
                params.qam = tp.tp_dtmb.symbol_rate;// bandwidth: 0-8M, 2-6M
                params.type_1501 = DTMB;
            }
            else
            {// for dvbc mode
                params.fre = tp.frequency;
                params.symb = tp.tp_dtmb.symbol_rate;
                params.qam = tp.tp_dtmb.modulation;
                params.type_1501 = DTMB_C;
            }
            params.type = FRONTEND_DTMB;
            break;
#endif
        default:
            params.type = FRONTEND_DVB_S;
    }

    app_set_tp(sat.tuner, &params, SET_TP_AUTO);
    return 0;
}

static int _dvb2ip_set_diseqc(GxBusPmDataSat sat, GxBusPmDataTP tp)
{
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};

    if(0 == sat.id || 0 == tp.id)
        return -1;

    //diseqc1.0
    diseqc10.type = DiSEQC10;
    diseqc10.u.params_10.chDiseqc = sat.sat_s.diseqc10;
    diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&sat);

    if(SEC_VOLTAGE_ALL == diseqc10.u.params_10.bPolar)
        diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&tp);

    diseqc10.u.params_10.b22k = app_show_to_sat_22k(sat.sat_s.switch_22K, &tp);

    // for polar
    AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
    app_ioctl(sat.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

    // for 22k
    AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
    app_ioctl(sat.tuner, FRONTEND_22K_SET, (void*)&_22K);

    app_set_diseqc_10(sat.tuner,&diseqc10, true);

    //diseqc1.1
    diseqc11.type = DiSEQC11;
    diseqc11.u.params_11.chUncom = sat.sat_s.diseqc11;
    diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
    diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
    diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
    app_set_diseqc_11(sat.tuner, &diseqc11, true);

    //post Sem
    GxFrontendOccur(sat.tuner);
    return 0;
}

int app_dvb2ip_set_tp(int prog_id)
{
    GxBusPmDataSat sat = {0};
    GxBusPmDataTP tp = {0};
    GxBusPmDataProg prog = {0};

    if(0 == prog_id
            || GXCORE_ERROR == GxBus_PmProgGetById(prog_id, &prog)
            || GXCORE_ERROR == GxBus_PmTpGetById(prog.tp_id, &tp)
            || GXCORE_ERROR == GxBus_PmSatGetById(prog.sat_id, &sat))
    {
        DVB2IP_ERROR("parameter is invalid");
        return -1;
    }

    _dvb2ip_set_diseqc(sat, tp);
    return _dvb2ip_set_tp(sat, tp, prog);
}

int app_dvb2ip_set_play(int prog_id)
{
    GxBusPmDataProg prog = {0};
    int prog_pos = 0;

    if(g_AppPvrOps.state != PVR_DUMMY)
    {
        app_pvr_stop();
        extern void app_set_avcodec_flag(bool state);
        app_set_avcodec_flag(false);
    }

    if (app_get_prog_pos_by_id_all(prog_id, &prog_pos) < 0)
    {
        DVB2IP_ERROR("can not find play_prog!\n");
        return -1;
    }

#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    if(GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS
            || GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
    {
        if (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
        {
            GxBusPmViewInfo  view_info_temp = {0};
            // recover view mode & list, without skip prog
            app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
            view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
            g_AppPlayOps.play_list_create(&view_info_temp);
            g_AppChOps.update_prog_num();

            if (app_get_prog_pos_by_id_all(prog_id, &prog_pos) < 0)
            {
                DVB2IP_ERROR("again can not find play_prog!\n");
                return -1;
            }
        }

        g_AppPlayOps.program_stop();

        GUI_EndDialog("after wnd_full_screen");
        GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
    }
    else
    {
        GUI_EndDialog("after wnd_full_screen");
    }
#endif

    g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, prog_pos);
    GxBus_PmProgGetById(prog_id, &prog);
    g_AppFullArb.state.tv = (prog.service_type == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

#if BOUQUET_SUPPORT
    g_AppBat.start(&g_AppBat);
#endif

    return 0;
}

#endif

