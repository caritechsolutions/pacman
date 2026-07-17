/** \addtogroup gxplayer_subtitle
 *  @{
 */

#ifndef __GXPLAYER_SUBTITLE_H__
#define __GXPLAYER_SUBTITLE_H__

#include "gxplayer_define.h"

__BEGIN_DECLS

/**
 * 字幕数据编码类型
 */
typedef enum {
	PLAYER_SUB_ENC_UTF8     ,  ///< 文本字幕 UTF8 编码.
	PLAYER_SUB_ENC_ANSI     ,  ///< 文本字幕 ANSI 编码.
	PLAYER_SUB_ENC_UCS      ,  ///< 文本字幕 UNICODE 编码.
	PLAYER_SUB_ENC_SPU         ///< 像素点字幕.
} PlayerSubEncode;

/**
 * 字幕类型
 */
typedef enum {
	PLAYER_SUB_TYPE_DVB     ,  ///< DVB 字幕.
	PLAYER_SUB_TYPE_FILE    ,  ///< FILE 外挂字幕.
	PLAYER_SUB_TYPE_INSIDE  ,  ///< 内置字幕.
	PLAYER_SUB_TYPE_ARIB_CC ,  ///< ARIB-CC 字幕.
	PLAYER_SUB_TYPE_ATSC_CC ,  ///< ATSC-CC 字幕.
	PLAYER_SUB_TYPE_SCTE    ,  ///< SCTE-CC 字幕.
	PLAYER_SUB_TYPE_DVB_TTX ,  ///< Teletext-Text 字幕.
	PLAYER_SUB_TYPE_DVB_MAG ,  ///< Teletext-Magazine 字幕.
} PlayerSubType;

/**
 * ATSC-CC 字幕业务类型
 */
typedef enum {
	PLAYER_ATSC_608_CC1 = 0,       ///< 608 CC1 业务字幕.
	PLAYER_ATSC_608_CC2,           ///< 608 CC2 业务字幕.
	PLAYER_ATSC_608_CC3,           ///< 608 CC3 业务字幕.
	PLAYER_ATSC_608_CC4,           ///< 608 CC4 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE1,  ///< 708 Service1 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE2,  ///< 708 Service2 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE3,  ///< 708 Service3 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE4,  ///< 708 Service4 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE5,  ///< 708 Service5 业务字幕.
	PLAYER_ATSC_708_DTV_SERVICE6,  ///< 708 Service6 业务字幕.
	PLAYER_ATSC_CC_SERVICE_AUTO,   ///< 自动选择第一次解析的业务字幕显示.
	PLAYER_ATSC_CC_SERVICE_INVALID ///< 解析字幕,但不显示字幕.
} PlayerATSCServiceID;

/**
 * 字幕标识参数
 */
typedef struct {
	int32_t                 pid;   ///< 字幕唯一标识.
	int16_t                 major; ///< 字幕主要信息: Teletext-Text, Teletext-Magazine 特有的信息.
	int16_t                 minor; ///< 字幕次要信息: Teletext-Text, Teletext-Magazine 特有的信息.
} PlayerSubPID;

/**
 * 字幕信息参数
 */
typedef struct {
	int                     id;                                ///< 字幕索引.
	char                    lang[PLAYER_TARCK_LANG_LONG];      ///< 字幕语言描述: 最大32个字符.
	char                    track_name[PLAYER_TARCK_NAME_LONG];///< 字幕信息描述: 最大32个字符.
	char                    codec_id[PLAYER_TARCK_CODEC_LONG]; ///< 字幕编码描述: 最大32个字符.
	PlayerSubEncode         encode;                            ///< 字幕数据编码.
	PlayerSubType           type;                              ///< 字幕类型.
	PlayerSubPID            pid;                               ///< 字幕标识.
} PlayerSubTrack;

/**
 * 字幕参数
 */
typedef struct  {
	void*                   sub_hdr;                    ///< 字幕私有信息,不可操作.
	int                     sub_num;                    ///< 字幕个数.
	PlayerSubTrack          track[PLAYER_MAX_TRACK_SUB];///< 字幕信息,最大支持32个字幕.
} PlayerSubtitle;

typedef struct{
	PlayerSubType           type;       ///< 字幕类型.
	const char*             file_name;  ///< 外挂文件名称,绝对路径.
	PlayerSubPID            pid;        ///< 字幕标识,外挂字幕不使用此信息.
	PlayerATSCServiceID     service_id; ///< ATSC-CC 业务类型.
} PlayerSubPara;

/**
 * \brief   加载字幕
 * \details 1. 加载字幕前,播放器需要先播放节目,一个播放器只能加载一个字幕.
 * \details 2. 重新加载字幕前,需要卸载已经加载的字幕.
 * \details 3. 加载 PLAYER_SUB_TYPE_FILE 外挂字幕, subpara.filename 指定文件路径名称.
 * \details 4. DVB 下的字幕,可以通过搜台得到字幕信息, subpara.pid.pid 为搜台 pid.
 * \details 5. 其他字幕,可以通过播放器得到字幕信息, subpara.pid.pid 为字幕索引.
 *
 * \param   name    [in] 播放器名称
 * \param   subpara [in] 加载字幕参数,详见: gxdocref PlayerSubPara.
 * \retval  非NULL       成功,字幕操作句柄.
 * \retval    NULL       失败.
 */
extern PlayerSubtitle*  GxPlayer_MediaSubLoad(const char* name, PlayerSubPara* subpara);

/**
 * \brief   卸载字幕.
 * \details 1. 卸载字幕前,播放器需要先加载一个字幕, 否则卸载失败.
 *
 * \param   name    [in]    播放器名称.
 * \param   subdata [in]    加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubUnLoad(const char* name, PlayerSubtitle* subdata);

/**
 * \brief   隐藏字幕.
 * \details 1. 隐藏正在显示的字幕,通过关闭 SPP 层达到隐藏效果,实际字幕还在解析播放.
 *
 * \param   name    [in]    播放器名称.
 * \param   subdata [in]    加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubHide(const char* name, PlayerSubtitle* subdata);

/**
 * \brief   显示字幕.
 * \details 1. 显示已经隐藏的字幕,通过开启 SPP 层达到显示效果.
 *
 * \param   name    [in]    播放器名称.
 * \param   subdata [in]    加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubShow(const char* name, PlayerSubtitle* subdata);

/**
 * \brief   字幕时间校准.
 * \details 1. 设置字幕提前显示或推迟显示,下一条字幕起生效,字幕重新加载后失效.
 *
 * \param   name    [in]    播放器名称.
 * \param   subdata [in]    加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \param   timems  [in]    提前/推迟显示的时长,单位毫秒: timems > 0 - 推迟播放, timems < 0 - 提前播放。
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubSync(const char* name, PlayerSubtitle* subdata, int timems);

/**
 * \brief   切换字幕.
 * \details 1. 切换字幕功能只对 DVB, 外挂,内置字幕有效,其余无效.
 * \details 2. 接口废弃.
 *
 * \param   name    [in]    播放器名称.
 * \param   subdata [in]    加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \param   pid     [in]    字幕标识,详见: gxdocref PlayerSubPID.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubSwitch(const char* name, PlayerSubtitle* subdata, PlayerSubPID pid);

/**
 * \brief   切换字幕显示制式.
 * \details 1. 接口废弃.
 *
 * \param   name       [in] 播放器名称.
 * \param   subdata    [in] 加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \param   resolution [in] 字幕显示制式.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubResolution(const char* name, PlayerSubtitle* subdata, int resolution);

/**
 * \brief   获取字幕当前时间和总时长.
 * \details 1. 只对 外挂字幕有效,其余无效.
 *
 * \param   name       [in] 播放器名称.
 * \param   subdata    [in] 加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \param   curtime   [out] 字幕当前时间.
 * \param   duration  [out] 字幕总时长.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubGetTime(const char* name,
		PlayerSubtitle* subdata,
		uint64_t *curtime,
		uint64_t*duration);

/**
 * \brief   跳转字幕到设置的时间.
 * \details 1. 只对 外挂字幕有效,其余无效.
 *
 * \param   name       [in] 播放器名称.
 * \param   subdata    [in] 加载字幕返回字幕类型,详见: gxdocref PlayerSubtitle.
 * \param   localtime  [in] 字幕跳转时间.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 */
extern status_t GxPlayer_MediaSubGotoLocalTime(const char* name,
		PlayerSubtitle* subdata,
		uint64_t localtime);

__END_DECLS

#endif

/** @}*/
