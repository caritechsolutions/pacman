/** \addtogroup gxsubtitle
 *  @{
 */

#ifndef __GX_SUBTITLE_ATSC_CC_H__
#define __GX_SUBTITLE_ATSC_CC_H__

/**
 * ATSC-CC 的业务类型
 */
typedef enum {
	GX_SUBTITLE_ATSC_608_CC1 = 0,      ///< 显示 608 ATSC-CC CC1 字幕.
	GX_SUBTITLE_ATSC_608_CC2,          ///< 显示 608 ATSC-CC CC2 字幕.
	GX_SUBTITLE_ATSC_608_CC3,          ///< 显示 608 ATSC-CC CC3 字幕.
	GX_SUBTITLE_ATSC_608_CC4,          ///< 显示 608 ATSC-CC CC4 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE1, ///< 显示 708 ATSC-CC Service1 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE2, ///< 显示 708 ATSC-CC Service2 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE3, ///< 显示 708 ATSC-CC Service3 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE4, ///< 显示 708 ATSC-CC Service4 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE5, ///< 显示 708 ATSC-CC Service5 字幕.
	GX_SUBTITLE_ATSC_708_DTV_SERVICE6, ///< 显示 708 ATSC-CC Service6 字幕.
	GX_SUBTITLE_ATSCCC_CC_AUTO,        ///< 自动显示.
	GX_SUBTITLE_ATSCCC_CC_INVAILD,     ///< 不显示.
} GX_SUBTITLE_ATSCCC_SERVICE;

/**
 * ATSC-CC 的业务参数
 */
typedef struct {
	GX_SUBTITLE_ATSCCC_SERVICE cur_service;         ///< 当前播放的 ATSC-CC 业务类型.
	unsigned int               num;                 ///< ATSC-CC 业务类型的总数量.
#define ATSCCC_NUM (16)                             ///< 最大支持 ATSC-CC 业务类型的数量.
	GX_SUBTITLE_ATSCCC_SERVICE service[ATSCCC_NUM]; ///< ATSC-CC 业务类型.
} GX_SUBTITLE_ATSCCC_INFO;

/**
 * \brief   获取 ATSC-CC 业务信息.
 * \details 1. 无法在视频流中提前获字幕业务类型,需要预先加载后,调用该接口获取实时更新的业务信息.
 *
 * \param   info [in] 业务类型参数.
 *
 * \retval   0        成功.
 * \retval  -1        失败.
 */
int GxSubtitle_ATSCGetServiceInfo(GX_SUBTITLE_ATSCCC_INFO *info);

/**
 * \brief   切换显示 ATSC-CC 业务字幕.
 *
 * \param   service [in] 业务类型参数.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
int GxSubtitle_ATSCSwitchService(GX_SUBTITLE_ATSCCC_SERVICE service);

#endif

/** @}*/
