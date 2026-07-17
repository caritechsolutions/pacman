#include "app_default_params.h"
#if MEDIA_SUBTITLE_SUPPORT
#include "pmp_setting.h"
#include "pmp_dvb_subt.h"
#include "play_manage.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif

static PlayerSubtitle *thiz_subt_handle = NULL;
extern int app_get_subt_real_select(int select);

status_t movie_dvb_subt_stop(void)
{
    if(thiz_subt_handle != NULL)
    {
        GxPlayer_MediaSubUnLoad(PMP_PLAYER_AV, thiz_subt_handle);
        thiz_subt_handle = NULL;
    }
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_deinit();
#endif

    return GXCORE_SUCCESS;
}

status_t movie_dvb_subt_start(uint8_t subt_index)
{
    PlayerSubPara subpara;

    movie_dvb_subt_stop();
    memset(&subpara, 0 , sizeof(PlayerSubPara));
    subpara.type = PLAYER_SUB_TYPE_DVB;
    subpara.pid.pid = app_get_subt_real_select(subt_index);
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_init();
#endif
    thiz_subt_handle = GxPlayer_MediaSubLoad(PMP_PLAYER_AV, &subpara);

    return GXCORE_SUCCESS;
}

status_t movie_dvb_subt_switch(uint8_t subt_index)
{
    PlayerSubPID pid = {0};

    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    pid.pid = app_get_subt_real_select(subt_index);

    return GxPlayer_MediaSubSwitch(PMP_PLAYER_AV, thiz_subt_handle, pid);
}

status_t movie_dvb_subt_hide(void)
{
    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    return GxPlayer_MediaSubHide(PMP_PLAYER_AV, thiz_subt_handle);
}

status_t movie_dvb_subt_show(void)
{
    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    return GxPlayer_MediaSubShow(PMP_PLAYER_AV, thiz_subt_handle);
}
#endif
