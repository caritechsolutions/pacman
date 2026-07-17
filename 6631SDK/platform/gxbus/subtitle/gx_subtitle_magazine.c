#include <gxcore.h>
#include "gx_subtitle_magazine.h"
#include "libzvbi/libzvbi.h"
#include "teletext/gx_magazine.h"
#include "teletext/gx_modifychar.h"
#include "gx_subtitle_setting.h"
#include "gx_subtitle_config.h"
#include "gx_avflow.h"

int GxSubtitle_MagFirstPage(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetFirstPage(&pgno);

	return GxMagazine_JumpPage(pgno);
}

int GxSubtitle_MagNextPage(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentPage    (&pgno);

	GxMagazine_BackwardSearchPage(&pgno);

	return GxMagazine_JumpPage(pgno);
}

int GxSubtitle_MagPrevPage(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentPage   (&pgno);

	GxMagazine_ForwardSearchPage(&pgno);

	return GxMagazine_JumpPage(pgno);
}

int GxSubtitle_MagNextSubPage(void)
{
	unsigned int subno;
	int ret = 0;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentSubPage(&subno);

	ret = GxMagazine_BackwardSearchSubPage(&subno);
	if (ret == 0)
		return GxMagazine_JumpSubPage(subno);
	else if (ret == 1)
		return GxMagazine_AutoJumpSubPage();

	return ret;
}

int GxSubtitle_MagPrevSubPage(void)
{
	unsigned int subno;
	int ret = 0;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentSubPage(&subno);

	ret = GxMagazine_ForwardSearchSubPage(&subno);
	if (ret == 0)
		return GxMagazine_JumpSubPage(subno);
	else if (ret == 1)
		return GxMagazine_AutoJumpSubPage();

	return ret;
}

int GxSubtitle_MagJumpPage(unsigned int pgno)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, pgno);

	return GxMagazine_JumpPage(pgno);
}

int GxSubtitle_MagSetPageLang(GX_SUBTITLE_PGLANG *pglang)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	return GxMagazine_SetPageLang(pglang);
}

int GxSubtitle_MagShowString(char str[7])
{
	av_flow_log("[Flow] - [%s %d]: %s\n", __func__, __LINE__, str);

	return GxMagazine_ShowString(str);
}

unsigned int GxSubtitle_MagGetFirstPgNo(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetFirstPage(&pgno);

	return pgno;
}

unsigned int GxSubtitle_MagGetCurrentPgNo(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentPage(&pgno);

	return pgno;
}

unsigned int GxSubtitle_MagGetCurrenSubPgNo(void)
{
	unsigned int subno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_GetCurrentSubPage(&subno);

	return subno;
}

unsigned int GxSubtitle_MagGetNextPgNo(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_BackwardSearchPage(&pgno);

	return pgno;
}

unsigned int GxSubtitle_MagGetPrevPgNo(void)
{
	unsigned int pgno;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxMagazine_ForwardSearchPage(&pgno);

	return pgno;
}

int GxSubtitle_MagGetSixLinkPgNo(unsigned int pgno[6])
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	return GxMagazine_GetSixLinkPgNo(pgno);
}

int GxSubtitle_MagSetOpacity(GX_SUBTITLE_MAG_OPACITY opacity)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, opacity);

	switch (opacity) {
	case GX_SUBTITLE_MAG_DEFAULT_OPACITY:
		subtitleConfig.magAlpha.user = 0;
		break;
	case GX_SUBTITLE_MAG_FULL_OPACITY:
		subtitleConfig.magAlpha.user  = 1;
		subtitleConfig.magAlpha.alpha = ((0x3 << 6) | 0x3f);
		break;
	case GX_SUBTITLE_MAG_WELL_OPACITY:
		subtitleConfig.magAlpha.user  = 1;
		subtitleConfig.magAlpha.alpha = ((0x2 << 6) | 0x3f);
		break;
	case GX_SUBTITLE_MAG_SEMI_OPACITY:
		subtitleConfig.magAlpha.user  = 1;
		subtitleConfig.magAlpha.alpha = ((0x1 << 6) | 0x3f);
		break;
	case GX_SUBTITLE_MAG_NONE_OPACITY:
		subtitleConfig.magAlpha.user = 1;
		subtitleConfig.magAlpha.alpha = ((0x0 << 6) | 0x3f);
		break;
	default:
		subtitleConfig.magAlpha.user = 0;
		break;
	}

	return GxMagazine_RefreshCurPage();
}

int GxSubtitle_VBIDebugChar(GX_SUBTITLE_VBI_DCHAR dchar)
{
	return GxMagazine_VBIDebugChar(&dchar);
}

int GxSubtitle_VBIModifyChar(GX_SUBTITLE_VBI_MCHAR mchar)
{
	return GxMagazine_VBIModifyChar(&mchar);
}

int GxSubtitle_CZEModifyChar(void)
{
	return GxMagazine_CZEModifyChar();
}
