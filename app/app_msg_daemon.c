#include "app.h"
#include "gxmsg.h"
#include "app_module.h"
#include "app_epg.h"
#include "app_wnd_search.h"
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#endif

extern uint32_t app_search_get_msg_process_si_subtable(void);
extern void app_search_clr_msg_process_si_subtable(void);


status_t AppDeamonServiceInit(handle_t self,int priority_offset)
{
    handle_t sch;

    GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_OK);
    GxBus_MessageListen(self, GXMSG_SI_SECTION_OK);
    GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);

    sch = GxBus_SchedulerCreate("AppDeamonMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 20, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    return GXCORE_SUCCESS;
}

void AppDeamonServiceDestroy(handle_t self)
{
    GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_OK);
    GxBus_MessageUnListen(self, GXMSG_SI_SECTION_OK);
    GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);

    GxBus_ServiceUnlink(self);

    return;
}

GxMsgStatus AppDeamonServiceRecvMsg(handle_t self, GxMessage * Msg)
{
    GxMsgStatus status = GXMSG_OK;

    switch (Msg->msg_id)
    {
        case GXMSG_SI_SUBTABLE_OK:
            {
                GxMsgProperty_SiSubtableOk *si_get = NULL;
                si_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiSubtableOk);

#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
                app_search_si_subtable_msg(Msg);
#endif

#if TKGS_SUPPORT
                    app_log_info("getSi_subtable ");
                if((g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_STOP) == 0)
                {
                    app_log_info("searchState = 1");
                    g_AppTkgsBackUpgrade.get(si_get);
                    break;
                }
                else
#endif
                {
                    g_AppPmt.get(&g_AppPmt, si_get);
                }

#if EMM_SUPPORT
                app_cat_subtable_ok(si_get);
#endif

            }
            break;

        case GXMSG_SI_SECTION_OK:
            {
#if ECM_SUPPORT ||EMM_SUPPORT || DUAL_CHANNEL_DESCRAMBLE_SUPPORT
                GxMsgProperty_SiSubtableOk *si_get = NULL;
                si_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiSubtableOk);
#endif
#if ECM_SUPPORT > 0
                g_AppEcm.get(&g_AppEcm, si_get);
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
                g_AppEcm2.get(&g_AppEcm2, si_get);
#endif

#endif


#if EMM_SUPPORT
                app_emm_get(si_get);
#endif
            }
            break;

        case GXMSG_SI_SUBTABLE_TIME_OUT:
#if TKGS_SUPPORT
            if((g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_STOP) == 0)
            {
                GxMsgProperty_SiSubtableOk *si_get = NULL;
                si_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiSubtableOk);
                //app_log_debug("func=%d line=%d\n",__FUNCTION__,__LINE__);
                g_AppTkgsBackUpgrade.timeout(si_get);
            }
#endif
            {
                GxParseResult *parse_result = NULL;
                parse_result = GxBus_GetMsgPropertyPtr(Msg,GxParseResult);
#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
                app_search_si_subtable_msg(Msg);
#endif

#if ECM_SUPPORT > 0
                g_AppEcm.timeout(&g_AppEcm, parse_result);
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
                g_AppEcm2.timeout(&g_AppEcm, parse_result);
#endif
#endif

#if EMM_SUPPORT
                app_emm_timeout(parse_result);
#endif

                GxBus_SiParserBufFree(parse_result->parsed_data);
            }
            break;

        default:
            break;
    }

    return status;
}

GxServiceClass app_deamon_service =
{
    "app deamon service",
    AppDeamonServiceInit,
    AppDeamonServiceDestroy,
    AppDeamonServiceRecvMsg,
    NULL,
};

