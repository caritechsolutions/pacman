#include "app_module.h"
#include "app.h"
#if ATSCCC_SUPPORT
#include "bus/module/subtitle/gx_subtitle_module.h"
#include "bus/module/subtitle/gx_subtitle_atsccc.h"

static   PlayerSubtitle* s_atsc_handle;
static GX_SUBTITLE_ATSCCC_INFO s_atsc_info = {0};
static bool s_astccc_mode = false;

static handle_t app_atsc_ShowTextOpen(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	return 0;
}

static void app_atsc_ShowTextClose(handle_t handle)
{
	return;
}

static int app_atsc_ShowTextDraw(handle_t handle, GxSubtitleShowTextPage *txt)
{
	return 0;
}

static int app_atsc_ShowTextClean(handle_t handle)
{
	return 0;
}

static int app_atsc_ShowTextSetScreenColor(handle_t handle, unsigned int color)
{
	return 0;
}

static int app_atsc_ShowTextSetDisplayPos(handle_t handle, GxSubtitleShowDisplay *pos)
{
	return 0;
}

static int app_atsc_ShowTextSetFont(handle_t handle, GxSubtitleShowFont *font)
{
	return 0;
}

static int app_atsc_ShowTextShow(void)
{
	return 0;
}

static int app_atsc_ShowTextHide(void)
{
	return 0;
}

static int app_atsc_ShowTextSetScreen(GxSubtitleShowScreen *screen)
{
	return 0;
}

void app_atsc_RegisterShowText(GxSubtitleShowTextOps ops)
{
    GxSubtitle_ModuleRegisterShowText(&ops);
}

void app_atsc_UnRegisterShowText(void)
{
    GxSubtitle_ModuleUnRegisterShowText();
}

void app_atsc_ConfigAtscccMode(void)
{
    GxSubtitle_ConfigAtscccMode(GX_SUBTITLE_ATSC_TEXT_SHOW);//矢量字库使用
}

void app_atsc_mode_set(bool flag)
{
    s_astccc_mode = flag;
}

bool app_atsc_mode_get(void)
{
    return s_astccc_mode;
}

void app_atsc_load(void)
{
    PlayerSubPara para;
    para.type = PLAYER_SUB_TYPE_ATSC_CC;
    para.service_id = PLAYER_ATSC_CC_SERVICE_INVALID;
    s_atsc_handle = GxPlayer_MediaSubLoad(PLAYER_FOR_NORMAL, &para);
}

void app_atsc_get_info(void)
{
    GxSubtitle_ATSCGetServiceInfo(&s_atsc_info);
}

unsigned int app_atsc_get_num(void)
{
    app_atsc_get_info();
    return s_atsc_info.num;
}

void app_atsc_switchservice(int sel)
{
    GX_SUBTITLE_ATSCCC_SERVICE service = s_atsc_info.service[sel];
    GxSubtitle_ATSCSwitchService(service);
}

void app_atsc_close_sub(void)
{
     GX_SUBTITLE_ATSCCC_SERVICE service = PLAYER_ATSC_CC_SERVICE_INVALID;
     GxSubtitle_ATSCSwitchService(service);
}

void app_atsc_unload(void)
{
    GxPlayer_MediaSubUnLoad(PLAYER_FOR_NORMAL, s_atsc_handle);
}

GxSubtitleShowTextOps gAtscTextOps = {
	.name   = "atsccc Show Text",
	.open   = app_atsc_ShowTextOpen,
	.close  = app_atsc_ShowTextClose,
	.draw   = app_atsc_ShowTextDraw,
	.clean  = app_atsc_ShowTextClean,
	.show   = app_atsc_ShowTextShow,
	.hide   = app_atsc_ShowTextHide,
	.setScreenColor = app_atsc_ShowTextSetScreenColor,
	.setScreen      = app_atsc_ShowTextSetScreen,
	.setDisplayPos  = app_atsc_ShowTextSetDisplayPos,
	.setFont        = app_atsc_ShowTextSetFont,
};

#endif
