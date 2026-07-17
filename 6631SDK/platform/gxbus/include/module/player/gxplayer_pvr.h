/** \addtogroup gxplayer_pvr
 **  @{
 **/
#ifndef __GXPLAYER_PVR_H__
#define __GXPLAYER_PVR_H__

#include "gxplayer_module.h"

__BEGIN_DECLS
/**
 * PVR 录制文件配置参数
 **/
typedef struct {
	int volume_sizemb;   ///< 录制文件每卷的大小,单位MB.
	int volume_maxnum;   ///< 录制文件卷的最大数量.
	int volume_fullstop; ///< 录制文件达到录制卷的最大数量时: 1 - 停止录制; 其他 - 循环录制覆盖最先录制的卷.
	int reserve_sizemb;  ///< 录制预留空间大小,剩余空间小于此大小时,停止录制,单位MB.
	int volume_maxtimes; ///< 最大录制时间,录制文件的可用播放时间小于此配置时间,同时需要录制有足够空间.
} PlayerPVRConfig;

/**
 * PVR 时移配置参数
 **/
typedef struct {
	int           enable;                 ///< 时移使能: 0 - 播放器暂停不进入时移; 非0 - 播放器暂停进入时移.
	unsigned char url[PLAYER_URL_LONG+1]; ///< 时移录制文件 URL (绝对路径), URL 参数参考: gxdocref.
	int           shift_dmxid;            ///< 时移录制解复用器 ID, 解复用器信息参考: gxdocref.
} PlayerShiftConfig;

/**
 * PVR 播放参数
 **/
typedef struct {
	int                     demux_id; ///< PVR 播放路解复用器 ID.
} PlayerPlayPVRConfig;

/**
 * PVR 录制信息扩展参数
 **/
typedef struct {
#define GX_DEMUXER_MAX_EXTPID_NUM (32) ///< 最大支持扩展录制通路个数.
	unsigned int is_rawts;             ///< 是否整个频点录制: 1 - 后续参数配置无效; 0 - 后续配置参数有效.
	unsigned int is_encrypt;           ///< 是否加扰录制,针对加扰节目: 1 - 加扰录制,参数 ext_encrypt 无效; 0 - 是否清流录制依赖参数 ext_encrypt.
	unsigned int is_reconly;           ///< 是否 DEMUX_SLOT_PSI 录制: 1 - 参数 ext_types 配置无效; 0 - 参数 ext_types 配置有效.
	unsigned int service_id;           ///< 录制服务 ID 参数,此参数不对录制产生影响.
	unsigned int ext_data[4];          ///< 用户记录私有参数.
	unsigned int ext_num;              ///< 扩展录制通路个数.
	unsigned int ext_pids[GX_DEMUXER_MAX_EXTPID_NUM];    ///< 扩展录制通路 PID 参数.
	unsigned int ext_types[GX_DEMUXER_MAX_EXTPID_NUM];   ///< 扩展录制通路类型,包括: DEMUX_SLOT_AUDIO, DEMUX_SLOT_VIDEO, DEMUX_SLOT_PES, DEMUX_SLOT_PSI.
	unsigned int ext_encrypt[GX_DEMUXER_MAX_EXTPID_NUM]; ///< 扩展录制通路是否加扰: 1 - 加扰; 0 - 清流.
	PlayerMediaContent ext_pids_content[GX_DEMUXER_MAX_EXTPID_NUM]; ///< 扩展录制通路数据类型.
	unsigned int ext_pids_content_type[GX_DEMUXER_MAX_EXTPID_NUM];  ///< 扩展录制通路数据类型对应数据编码.
																	///< 字幕编码参考: gxdocref PlayerSubType.
																	///< 音频编码参考: gxdocref AudioCodecType.
																	///< 视频编码参考: gxdocref VideoCodecType.
	unsigned int ext_pids_major[GX_DEMUXER_MAX_EXTPID_NUM]; ///< 扩展录制通路 PID 的主要信息, Teletext 字幕包含该信息.
	unsigned int ext_pids_minor[GX_DEMUXER_MAX_EXTPID_NUM]; ///< 扩展录制通路 PID 的次要信息, Teletext 字幕包含该信息.
	unsigned int ext_pids_codec[GX_DEMUXER_MAX_EXTPID_NUM]; ///< 扩展录制通路数据的编码信息,此参数用户私有信息.
	unsigned int ext_pids_name[GX_DEMUXER_MAX_EXTPID_NUM];  ///< 扩展录制通路的名称,此参数用户私有信息.
	unsigned int ext_pids_lang[GX_DEMUXER_MAX_EXTPID_NUM];  ///< 扩展录制通路的语言,此参数用户私有信息.
} GxDemuxerRecordExtInfo;

/**
 * PVR 录制信息用户参数
 **/
typedef struct {
#define RECODER_USER_MAX_COUNT 32                            ///< 最大输入 TS 包的个数
#define RECODER_USER_MAX_LEN   (RECODER_USER_MAX_COUNT*188)  ///< 最大输入 TS 包的长度.
	unsigned int have_pmt;                                   ///< 输入 TS 包是否包含 PMT 表.
	unsigned int userdata_len;                               ///< 输入 TS 包的长度.
	char         userdata[RECODER_USER_MAX_LEN];             ///< 输入 TS 包的数组.
} GxDemuxerRecordUserInfo;

/**
 * PVR 录制信息用户参数
 **/
typedef struct {
	GxDemuxerRecordExtInfo  ext_info; ///< 录制信息扩展参数.
	GxDemuxerRecordUserInfo ext_data; ///< 录制信息用户参数.
} PlayerRecordConfig;

/**
 * PVR 录制信息加密参数
 **/
typedef struct {
	unsigned int user_encrypt_enable;    ///< 用户加密使能开关.
	unsigned int user_encrypt_blocksize; ///< 用户加密时加密块大小.
} PlayerRecordEncrypt;

/**
 * \brief   配置 PVR 录制文件参数.
 * \details 1. 播放器初始化后任意时间可以使用,参数下一次开启 PVR 录制生效.
 * \details 2. 配置参数对非 ".pvr" 后缀的文件有效,包括: 时移以及录制.
 * \details 3. 使用场景: 循环 PVR 功能,需要配置分卷信息,卷的总个数不少于 2 卷,且最大播放卷数: 卷的总个数 - 1.
 *
 * \param   config                 [in] 配置时移或录制文件参数参考: gxdocref PlayerPVRConfig.
 * \param   config.volume_sizemb   [in] 默认值: 200M; 0 - 无限制卷的大小录制,写失败或剩余空间小于预留空间时停止录制.
 * \param   config.volume_maxnum   [in] 默认值: 0; 0 - 无限制卷的个数录制,写失败或剩余空间小于预留空间时停止录制.
 * \param   config.volume_fullstop [in] 默认值: 0.
 * \param   config.reserve_sizemb  [in] 默认值: 20M.
 * \param   config.volume_maxtimes [in] 默认值: 0; 0 - 无限制最大录制时间.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
status_t GxPlayer_SetPVRConfig(PlayerPVRConfig* config);

/**
 * \brief   获取 PVR 录制文件配置参数.
 * \details 1. 播放器初始化后任意时间可以使用,获取参数为上一次配置信息.
 *
 * \param   config [out]    获取时移或录制文件参数参考: gxdocref PlayerPVRConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_GetPVRConfig(PlayerPVRConfig* config);

/**
 * \brief   配置 PVR 时移参数.
 * \details 1. 播放器初始化后任意时间可以使用,参数下一次开启 PVR 时移生效.
 *
 * \param   config [in]     配置时移参数参考: gxdocref PlayerShiftConfig.
 *                          config.enable:      默认值: 0.
 *                          config.url:         默认值: "/mnt/usb0_1/shift.ts".
 *                          config.shift_dmxid: 默认值: 0.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_SetTimeShiftConfig(PlayerShiftConfig* config);

/**
 * \brief   获取 PVR 时移参数.
 * \details 1. 播放器初始化后任意时间可以使用,获取参数为上一次配置信息.
 * \details 2. PVR 播放包括: 时移和回放.
 *
 * \param   config [out]    获取时移参数参考: gxdocref PlayerShiftConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_GetTimeShiftConfig(PlayerShiftConfig* config);

/**
 * \brief   配置 PVR 播放参数.
 * \details 1. 播放器初始化后任意时间可以使用,参数下一次开启 PVR 播放生效.
 *
 * \param   config             [in] 配置播放参数参考: gxdocref PlayerPlayPVRConfig.
 * \param   config.shift_dmxid [in] 默认值: 1.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_SetPlayPVRConfig(PlayerPlayPVRConfig *config);

/**
 * \brief   启动一个播放器(包括录制器)录制一路节目.
 * \details 1. 可以启动多个播放器进行录制节目,一个播放器最多只能录制一路节目,播放器名称唯一标识.
 * \details 2. 启动录制时,播放器处于非录制状态时才能正常录制节目,否则失败.
 * \details 3. 启动录制时,播放器录制成功时会申请播放器资源,需要通过 GxPlayer_MediaStop 才能释放播放器资源.
 * \details 4. 启动录制后,可以通过配置相同的播放源和录制文件进入时移.
 *
 * \param   name [in]       播放器名称.
 * \param   url  [in]       播放源的 URL, URL 描述参考: gxdocref.
 * \param   file [in]       录制文件的 URL, 绝对路径, URL 描述参考: gxdocref.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaRecord(const char* name, const char* url, const char* file);

/**
 * \brief   设置一个播放器录制的用户参数.
 * \details 1. 配置录制的用户参数时,播放器配置成功时会申请播放器资源,需要通过 GxPlayer_MediaStop 才能释放播放器资源.
 * \details 2. 配置录制的用户参数后,播放器名称和启动录制的播放器名称一致时,用户参数才对录制才生效.
 *
 * \param   name   [in]     播放器名称.
 * \param   config [in]     设置录制用户参数参考: gxdocref PlayerRecordConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaRecordConfig(const char* name, PlayerRecordConfig* config);

/**
 * \brief   通过文件名称获取文件的用户参数.
 * \details 1. 文件处于录制,时移或播放状态时,不建议使用该接口获取用户参数,会造成临时使用资源多一倍.
 *
 * \param   srcurl [in]     文件名称.
 * \param   config [out]    获取录制用户参数参考: gxdocref PlayerRecordConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetRecordConfigByUrl(const char* srcurl, PlayerRecordConfig* config);

/**
 * \brief   通过播放器名称获取文件的用户参数.
 * \details 1. 文件处于录制,时移或回放状态时,才可以使用该接口获取用户参数.
 *
 * \param   name   [in]     播放器名称.
 * \param   config [out]    获取录制用户参数参考: gxdocref PlayerRecordConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetRecordConfigByName(const char* name, PlayerRecordConfig* config);

/**
 * \brief   设置一个播放器录制的加密参数.
 * \details 1. 配置录制的用户参数时,播放器配置成功时会申请播放器资源,需要通过 GxPlayer_MediaStop 才能释放播放器资源.
 * \details 2. 配置录制的用户参数后,播放器名称和启动录制的播放器名称一致时,加密参数才对录制才生效.
 *
 * \param   name    [in]    播放器名称.
 * \param   encrpyt [in]    设置录制用户参数参考: gxdocref PlayerRecordEncrypt.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaRecordEncrypt(const char* name, PlayerRecordEncrypt* encrpyt);

/**
 * \brief   停止一个播放器的一个节目录制.
 * \details 1. 停止播放器录制,不会释放播放器的资源,需要GxPlayer_MediaStop才能释放播放器资源.
 * \details 2. 使用场景:时移下的停止录制,但不停止时移.
 *
 * \param   name   [in]     播放器名称.
 * \retval  GX_PLAYER_OK    成功
 * \retval  GX_PLAYER_ERROR 失败
 **/
extern status_t GxPlayer_MediaStopRecord(const char* name);

/**
 * pvr录制分段参数
 */
typedef struct {
	const char *desc;          ///<分段segment描述信息
} PlayerPvrSegmentInfo;

/**
 * pvr录制分段列表参数
 */
typedef struct {
	unsigned int count;       ///<分段segment个数
	const char **prefix;      ///<分段segment唯一标识的指针，例如: prefix[0]一个分段标识
} PlayerPvrSegmentList;

/**
 * pvr录制事件信息参数
 */
typedef struct {
	const char *desc;          ///<事件event描述信息
	const char *chapter;       ///<事件event节目信息,例如:流星花园
	const char *chapter_desc;  ///<事件event节目详细信息描述，例如: 主演xx
	const char *start_time;    ///<事件event节目开始时间，例如: 00:07:00
	const char *end_time;      ///<事件event节目结束时间，例如: 00:08:00
} PlayerPvrEventInfo;

/**
 * pvr录制事件列表参数
 */
typedef struct {
	unsigned int count;       ///<事件event个数
	const char **prefix;      ///<事件event唯一标识的指针，例如: prefix[0]一个事件标识
} PlayerPvrEventList;

/**
 * pvr录制事件集参数
 */
typedef struct {
	const char *desc;          ///<事件集list描述信息
} PlayerPvrListInfo;

/**
 * \brief   pvr录制时,创建新分段进行分段录制.
 * \details 启动录制前进行分段录制,播放器会根据播放器名称申请资源并保存分段录制信息.
 * \details 启动录制后进行分段录制,播放器会根据播放器名称创建新分段录制,保存上一次分段录制.默认保存分段前缀为"gxdefseg".
 * \details 启动录制进行分段录制,需要GxPlayer_MediaStop才能释放播放器资源.
 *
 * \param   name   [in] 播放器名称.
 * \param   prefix [in] 分段前缀,唯一标识,不超过16字节
 * \param   info   [in] 分段参数参考:gxdocref
 * \retval  0           成功
 * \retval -1           分段前缀超过16字节,或对应文件已存在录制中
 * \retval -2           创建文件或文件夹异常
 * \retval -3           申请不到录制分段资源
 */
extern status_t GxPVRPlayer_NewSegment(const char *name,
		const char *prefix,
		PlayerPvrSegmentInfo *info);

/**
 * \brief   pvr录制时,删除已经录制完成的分段.
 * \details 删除录制的分段,需要分段已完成录制,不释放播放器资源.
 * \details 删除录制的分段,需要启动录制后,停止录制前.
 *
 * \param   name   [in] 播放器名称
 * \param   prefix [in] 分段前缀,唯一标识
 * \retval  0           成功
 * \retval -1           分段前缀对应文件不存在,或已被删除
 * \retval -2           分段前缀对应文件正在录制中
 */
extern status_t GxPVRPlayer_DeleteSegment(const char *name,
		const char *prefix);

/**
 * \brief   pvr录制时,打开事件.
 * \details 打开事件,需要启动录制后,停止录制前.
 * \details 最大支持32个事件同时被打开.
 *
 * \param   name   [in] 播放器名称
 * \param   prefix [in] 事件前缀,唯一标识
 *          info   [in] 事件参数参考:gxdocref
 * \retval  0           成功
 * \retval -1           事件超过32个,或对应文件已存在录制中
 * \retval -2           创建文件或文件夹异常
 * \retval -3           申请不到录制事件资源
 */
status_t GxPVRPlayer_OpenEvent(const char *name,
		const char *prefix,
		PlayerPvrEventInfo *info);

/**
 * \brief   pvr录制时,关闭事件.
 * \details 关闭事件,需要启动录制后,停止录制前,事件被打开.
 *
 * \param   name   [in] 播放器名称
 * \param   prefix [in] 事件前缀,唯一标识
 * \retval   0          成功
 * \retval  -1          事件不存在,或已被删除,或者被关闭
 */
status_t GxPVRPlayer_CloseEvent(const char *name,
		const char *prefix);

/**
 * \brief   pvr录制时,删除事件.
 * \details 删除事件,需要启动录制后,停止录制前.
 * \details 删除事件,允许事件处于打开状态.
 *
 * \param  name   [in] 播放器名称
 * \param  prefix [in] 事件前缀,唯一标识
 * \retval  0          成功
 * \retval -1          事件不存在,或已被删除
 */
status_t GxPVRPlayer_DeleteEvent(const char *name,
		const char *prefix);

/**
 * \brief   pvr录制时,在打开的事件中添加单个或多个分段.
 * \details 事件中添加的分段,需要启动录制后,停止录制前.
 * \details 事件中添加的分段,需要分段存在,且不存在于事件中.
 * \details 事件中添加的分段,允许在事件中添加正在录制的分段,但该分段为事件最后一个分段.
 *
 * \param   name   [in] 播放器名称.
 * \param   prefix [in] 事件前缀,唯一标识.
 * \param   list   [in] 分段列表参数参考:gxdocref PlayerPvrSegmentList.
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVRPlayer_AddSegmentToEvent(const char *name,
		const char *prefix,
		PlayerPvrSegmentList *list);

/**
 * \brief   pvr录制时,在事件中移除单个或者多个分段.
 * \details 事件中移除的分段,需要启动录制后,停止录制前.
 * \details 事件中移除的分段,需要该分段存在于事件中.
 *
 * \param   name   [in] 播放器名称.
 * \param   prefix [in] 事件前缀,唯一标识.
 * \param   list   [in] 分段列表参数参考:gxdocref PlayerPvrSegmentList.
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVRPlayer_RemoveSegmentInEvent(const char *name,
		const char *prefix,
		PlayerPvrSegmentList *list);

/**
 * \brief   pvr录制时,打开事件集.
 * \details 打开事件集,需要启动录制后,停止录制前.
 * \details 最大支持32个事件集同时被打开.
 *
 * \param  name   [in] 播放器名称.
 * \param  prefix [in] 事件集前缀,唯一标识.
 * \param  info   [in] 事件集参数参考:gxdocref PlayerPvrListInfo.
 * \retval  0          成功
 * \retval -1          事件集已被打开
 * \retval -2          申请不到事件集资源
 */
status_t GxPVRPlayer_OpenList(const char *name,
		const char *prefix,
		PlayerPvrListInfo *info);

/**
 * \brief   pvr录制时,关闭事件集.
 * \details 关闭事件集,需要启动录制后,停止录制前,事件集被打开.
 *
 * \param  name   [in] 播放器名称.
 * \param  prefix [in] 事件集前缀,唯一标识.
 * \retval  0          成功
 * \retval -1          事件集已被关闭
 * \retval -2          申请不到事件集资源
 */
status_t GxPVRPlayer_CloseList(const char *name,
		const char *prefix);

/**
 * \brief   pvr录制时,删除事件集.
 * \details 删除事件集,需要启动录制后,停止录制前.
 * \details 删除事件集,允许事件集处于打开状态.
 *
 * \param   name   [in] 播放器名称.
 * \param   prefix [in] 事件集前缀,唯一标识.
 * \retval   0          成功.
 * \retval  -1          事件集不存在.
 */
status_t GxPVRPlayer_DeleteList(const char *name,
		const char *prefix);

/**
 * \brief   pvr录制时,在事件集中添加单个或多个事件.
 * \details 事件集中添加事件,需要启动录制后,停止录制前.
 * \details 事件集中添加事件,需要事件存在.
 *
 * \param   name   [in] 播放器名称.
 * \param   prefix [in] 事件集前缀,唯一标识.
 * \param   list   [in] 事件列表参数参考:gxdocref PlayerPvrEventList.
 * \retval   0          成功.
 * \retval  -1          失败.
 */
status_t GxPVRPlayer_AddEventToList(const char *name,
		const char *prefix,
		PlayerPvrEventList *list);

/**
 * \brief   pvr录制时,在事件集中移除单个或者多个事件.
 * \details 事件集中移除事件,需要启动录制后,停止录制前.
 * \details 事件集中移除事件,需要该事件存在于事件集中.
 *
 * \param   name   [in] 录制器名称.
 *          prefix [in] 事件集前缀,唯一标识.
 *          list   [in] 事件列表参数参考:gxdocref PlayerPvrEventList.
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVRPlayer_RemoveEventInList(const char *name,
		const char *prefix,
		PlayerPvrEventList *list);

/**
 * pvr 文件属性
 **/
typedef struct {
	const char *prefix;           ///<文件名称前缀.
	const char *file;             ///<文件,包括绝对路径.
} PvrFileAttr;

/**
 * pvr录制事件,事件集,分段文件个数,文件属性
 */
typedef struct {
	unsigned int    list_count;     ///<事件集list文件个数.
	PvrFileAttr    *list_array;     ///<事件集list文件属性数组.
	unsigned int    event_count;    ///<事件event文件个数.
	PvrFileAttr    *event_array;    ///<事件event文件属性数组.
	unsigned int    segment_count;  ///<分段segment文件个数.
	PvrFileAttr    *segment_array;  ///<分段segment文件属性数组.
} PvrPvrFile;

/**
 * pvr录制事件集描述信息,包括事件个数,事件文件属性.
 */
typedef struct {
	const char     *desc;         ///<事件集list描述信息
	unsigned int    event_count;  ///<事件event文件个数
	PvrFileAttr    *event_array;  ///<事件event文件属性数组
} PvrPvrList;

/**
 * pvr录制事件描述信息,包括分段个数,分段文件属性.
 */
typedef struct {
	const char         *desc;          ///<事件event描述,文件已存在desc字段,该参数无效.
	const char         *chapter;       ///<事件event节目信息,例如:流星花园,文件已存在chapter字段,该参数无效.
	const char         *chapter_desc;  ///<事件event节目信息描述，例如: 主演xx,文件已存在chapter_desc字段,该参数无效.
	const char         *start_time;    ///<事件event节目开始时间，例如: 00:07:00,文件已存在start_time字段,该参数无效.
	const char         *end_time;      ///<事件event节目结束时间，例如: 00:08:00,文件已存在end_time字段,该参数无效.
	unsigned long long  filesize;      ///<事件event节目未被移除的segment构成的节目总大小.
	unsigned long long  duration;      ///<事件event节目未被移除的segment构成的节目总时长.
	unsigned int        segment_count; ///<分段segment文件个数.
	PvrFileAttr        *segment_array; ///<分段segment文件属性数组.
} PvrPvrEvent;

/**
 * pvr录制分段描述信息.
 */
typedef struct {
	const char         *desc;        ///<分段segment描述.
	unsigned long long  filesize;    ///<分段segment播放大小.
	unsigned long long  duration;    ///<分段segment播放时长.
} PvrPvrSegment;

typedef enum {
	PVR_FILE_TYPE_PVR      = 0,      ///pvr     pvr 类型文件,对应文件后缀".pvr".
	PVR_FILE_TYPE_LIST     = 1,      ///pvr    list 类型文件,对应文件后缀".lst".
	PVR_FILE_TYPE_EVENT    = 2,      ///pvr   event 类型文件,对应文件后缀".evt".
	PVR_FILE_TYPE_SEGMENT  = 3,      ///pvr segment 类型文件,对应文件后缀".seg".
} PvrFileType;

/**
 * pvr录制事件和事件集信息
 */
typedef struct {
	PvrFileType       type;        ///pvr 类型标识,类型详细见: gxdocref PvrFileType.
	union {
		PvrPvrFile    file;        ///pvr    file 文件输出信息.
		PvrPvrList    list;        ///pvr    list 文件输出信息.
		PvrPvrEvent   event;       ///pvr   event 文件输出信息.
		PvrPvrSegment segment;     ///pvr segment 文件输出信息.
	};
	const char       *pvr_file;    ///文件所属的 ".pvr" 文件名称,绝对路径.
	const char       *dir;         ///文件对应创建的文件夹名称，绝对路径．注: ".pvr"/".seg" 有对应文件夹，".lst"/".evt"无对应文件夹．
	const char       *path;        ///文件所属的绝对路径．
	const char       *prefix;      ///文件对应的前缀．
} PvrPvrInfo;

/**
 * pvr录制事件和事件集编辑
 */
typedef union {
	PvrPvrList    list;           ///pvr  list 事件集编辑输入信息.
	PvrPvrEvent   event;          ///pvr event 事件编辑输入信息.
} PvrEventListEdit;

/**
 * \brief  获取pvr录制文件的分段,事件和事件集文件,以及文件信息
 * \details pvr录制文件后缀可以是: ".pvr",".lst",".evt",".seg"
 *
 * \param  file [in]  文件名称,绝对路径
 * \param  info [out] 输出信息,参数详细见: gxdocref PvrPvrInfo.
 * \retval  0         成功
 * \retval 其他       失败
 */
status_t GxPVR_GetPVRInfo(const char *file, PvrPvrInfo *info);

/**
 * \brief  释放获取的pvr录制信息资源.
 *
 * \param  info [in] 获取的pvr信息.
 * \retval  0         成功
 * \retval 其他       失败
 */
status_t GxPVR_FreePVRInfo(PvrPvrInfo *info);

/**
 * \brief  删除pvr录制文件.
 * \details pvr录制文件后缀可以是: ".pvr",".lst",".evt",".seg"
 *
 * \param  file [in]  文件名称,绝对路径．
 * \retval  0         成功
 * \retval 其他       失败
 */
status_t GxPVR_DeletePVRFile(const char *file);

/**
 * \brief  清除在event中不存在的 segment.
 * \details pvr录制文件后缀可以是: ".pvr"
 *
 * \param  pvr_file [in]  文件名称,绝对路径．
 * \retval  0             成功
 * \retval 其他           失败
 */
status_t GxPVR_ClearSegmentFileInEvent(const char *pvr_file);

/**
 * \brief   打开pvr录制文件,事件文件,事件集文件
 * \details pvr录制文件后缀可以是: ".lst", ".evt".
 *
 * \param  file [in] 文件名称,绝对路径
 * \retval  0       失败
 * \retval 其他     成功,返回操作句柄
 */
handle_t GxPVREventList_Open(const char *file);

/**
 * \brief   增加事件,事件集信息
 * \details 打开的文件后缀为".lst",编辑事件集,增加事件信息.
 * \details 打开的文件后缀为".evt",编辑事件,增加分段信息.
 *
 * \param   handle [in]  打开文件返回的操作句柄
 * \param   edit   [out] 事件集或者事件
 * \retval   0           成功
 * \retval  -1           失败
 */
status_t GxPVREventList_Add(handle_t handle, PvrEventListEdit *edit);

/**
 * \brief   删除事件,事件集信息
 * \details 文件后缀为".lst",编辑事件集,删除事件信息.
 * \details 文件后缀为".evt",编辑事件,删除分段信息.
 *
 * \param   handle [in]  打开文件返回的操作句柄
 * \param   edit   [out] 事件集或者事件
 * \retval   0           成功
 * \retval  -1           失败
 */
status_t GxPVREventList_Remove(handle_t handle, PvrEventListEdit *edit);

/**
 * \brief   关闭打开的pvr录制文件,事件文件,事件集文件
 *
 * \param   handle [in] 打开文件返回的操作句柄
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVREventList_Close(handle_t handle);

typedef enum {
	PLAYER_PVR_PVR_ENCRYPT = 0x1,                    ///<pvr 加解密方式
	PLAYER_PVR_DVR_ENCRYPT = 0x2,                    ///<dvr 加解密方式
} PlayerPvrEncryptMode;

typedef enum {
	PLAYER_PVR_BUF_NORMAL = 0,    //非保护BUF,CPU直接访问
	PLAYER_PVR_BUF_SECURE = 1     //保护BUF,CPU需要由安全模块
} PlayerPvrBufSecureType;

typedef struct {
	PlayerPvrEncryptMode   enc_mode;             ///<接解密方式.
	PlayerPvrBufSecureType ibuf_type;            ///<输入参数，输入buf是否保护，保护buf不能直接访问
	PlayerPvrBufSecureType obuf_type;            ///<输入参数，输出buf是否保护
	union {
		struct {
			unsigned char *i_buf;                ///<输入 buf 地址.
			unsigned int   i_size;               ///<输入 buf 大小.
			unsigned char *o_buf;                ///<输出 buf 地址.
			unsigned int   o_size;               ///<输出 buf 大小.
			char          *key_desc;             ///<描述 key 字符串, 加密时 key 为输出参数, 解密时 key 为输入参数.字符串中不能使用回车符'\n'.
		} pvr;
		struct {
			unsigned char *i_buf;                ///<输入 buf 地址.
			unsigned int   i_size;               ///<输入 buf 大小.
			unsigned char *o_buf;                ///<输出 buf 地址.
			unsigned int   o_size;               ///<输出 buf 大小.
			long long      fpos;                 ///<输入当前解密文件位置.
		} dvr;
	};
} PlayerPvrEncryptPara;

typedef enum {
	PLAYER_PVR_FUNC_SUCCESS =  0,               ///<加解密成功
	PLAYER_PVR_FUNC_FAILURE = -1,               ///<加解密失败
	PLAYER_PVR_FUNC_REPEAT  = -2,               ///<需要重新调用加解密函数
} PlayerPvrFuncType;

typedef struct {
	const char          *name;                          ///< 自定名称.
	const char          *author;                        ///< 自定作者.
	PlayerPvrEncryptMode enc_mode;                      ///< 加解密方式.
	PlayerPvrFuncType  (*encrpyt) (PlayerPvrEncryptPara *para);  ///< 加密回调函数.
	PlayerPvrFuncType  (*decrpyt) (PlayerPvrEncryptPara *para);  ///< 解密回调函数.
} PlayerPvrEncryptOps;

/**
 * \brief   注册 pvr 加解密回调函数类
 *
 * \param   ops [in]    回调函数类
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVRPlayer_RegisterEncrypt(PlayerPvrEncryptOps *ops);

/**
 * \brief   取消 pvr 加解密回调函数类
 *
 * \param   ops [in]    回调函数类
 * \retval   0          成功
 * \retval  -1          失败
 */
status_t GxPVRPlayer_UnRegisterEncrypt(PlayerPvrEncryptOps *ops);

__END_DECLS

#endif

/** @}*/
