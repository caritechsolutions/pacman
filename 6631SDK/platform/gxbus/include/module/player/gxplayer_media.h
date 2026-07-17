/** \addtogroup  gxplayer_media
 *  @{
 */
#ifndef __GXPLAYER_MEDIA_H__
#define __GXPLAYER_MEDIA_H__

__BEGIN_DECLS

/**
 * 播放器状态类型
 **/
typedef enum {
	PLAYER_STATUS_STOPPED              ,  ///< 播放器停止播放状态.
	PLAYER_STATUS_PLAY_START           ,  ///< 播放器启动播放状态,未启动音视频解码.
	PLAYER_STATUS_PLAY_PAUSE           ,  ///< 播放器暂停播放状态.
	PLAYER_STATUS_PLAY_RUNNING         ,  ///< 播放器播放状态,已启动音视频解码.
	PLAYER_STATUS_PLAY_END             ,  ///< 播放器结束播放状态.
	PLAYER_STATUS_RECORD_PAUSE         ,  ///< 播放器暂停录制状态.
	PLAYER_STATUS_RECORD_RUNNING       ,  ///< 播放器录制状态.
	PLAYER_STATUS_RECORD_FULL          ,  ///< 播放器录制满状态.
	PLAYER_STATUS_RECORD_END           ,  ///< 播放器结束录制状态.
	PLAYER_STATUS_SHIFT_START          ,  ///< 播放器启动时移状态.
	PLAYER_STATUS_SHIFT_PAUSE          ,  ///< 播放器时移暂停状态.
	PLAYER_STATUS_SHIFT_RUNNING        ,  ///< 播放器时移状态.
	PLAYER_STATUS_SHIFT_SWITCH         ,  ///< 播放器时移过程切换状态.
	PLAYER_STATUS_SHIFT_HOLD_PAUSE     ,  ///< 播放器暂停进入时移暂停状态.
	PLAYER_STATUS_SHIFT_HOLD_RUNNING   ,  ///< 播放器恢复进入时移播放状态.
	PLAYER_STATUS_SHIFT_END            ,  ///< 播放器退出时移播放状态.
	PLAYER_STATUS_ERROR                ,  ///< 播放器出错,见播放器错误码类型.
	PLAYER_STATUS_START_FILL_BUF       ,  ///< 播放器缓冲数据不足,暂停播放状态.
	PLAYER_STATUS_END_FILL_BUF         ,  ///< 播放器缓冲数据足够,进入播放状态.
	PLAYER_STATUS_SERVER_ERROR         ,  ///< 播放器服务器出错.
	PLAYER_STATUS_PLAY_SOF             ,  ///< 播放器快退到文件头状态.
} PlayerStatus;

/**
 * 播放器的EHE(Event Hook Executor)状态类型
 **/
typedef enum {
	PLAYER_EHE_STATUS_STOPPED              ,  ///< EHE状态：停止播放.
	PLAYER_EHE_STATUS_PLAY_START           ,  ///< EHE状态：开始播放.
	PLAYER_EHE_STATUS_PLAY_PAUSE           ,  ///< EHE状态：暂停播放.
	PLAYER_EHE_STATUS_PLAY_RUNNING         ,  ///< EHE状态：正在播放.
	PLAYER_EHE_STATUS_PLAY_END             ,  ///< EHE状态：结束播放.
	PLAYER_EHE_STATUS_ERROR                ,  ///< EHE状态：播放错误.
	PLAYER_EHE_STATUS_START_FILL_BUF       ,  ///< EHE状态：开始缓冲数据.
	PLAYER_EHE_STATUS_END_FILL_BUF         ,  ///< EHE状态：结束缓冲数据.
	PLAYER_EHE_STATUS_PLAY_RESUME          ,  ///< EHE状态：恢复播放.
	PLAYER_EHE_STATUS_BANDWIDTH_SWITCH     ,  ///< EHE状态：多带宽切换.
	PLAYER_EHE_STATUS_AUDIO_SWITCH         ,  ///< EHE状态：多音轨切换.
	PLAYER_EHE_STATUS_SUB_SWITCH           ,  ///< EHE状态：多字幕切换.
	PLAYER_EHE_STATUS_PLAY_SEEK            ,  ///< EHE状态：快进快退状态.
} PlayerHEHStatus;

/**
 * 播放器错误码类型
 **/
typedef enum {
	PLAYER_ERROR_NO_ERROR              , ///< 播放器正常.
	PLAYER_ERROR_NO_DATA_SOURCE        , ///< 播放器获取不到数据.
	PLAYER_ERROR_NO_DEMUX_TOOLS        , ///< 播放器不支持文件格式解复用.
	PLAYER_ERROR_NO_MUXER_TOOLS        , ///< 播放器不支持文件格式复用.
	PLAYER_ERROR_NO_DUMP_TOOLS         , ///< 播放器录制失败.
	PLAYER_ERROR_NO_VIDEO_DECODER      , ///< 播放器不支持的视频编码类型.
	PLAYER_ERROR_NO_AUDIO_DECODER      , ///< 播放器不支持的音频编码类型.
	PLAYER_ERROR_NO_SUB_DECODER        , ///< 播放器不支持的字幕编码类型.
	PLAYER_ERROR_NO_VIDEO_RENDER       , ///< 播放器不支持的视频输出类型.
	PLAYER_ERROR_NO_AUDIO_RENDER       , ///< 播放器不支持的音频输出类型.
	PLAYER_ERROR_NO_SUB_RENDER         , ///< 播放器不支持的字幕渲染类型.
	PLAYER_ERROR_DEMUX_ERROR           , ///< 播放器的解复用器配置失败.
	PLAYER_ERROR_VIDEO_DECODER_ERROR   , ///< 播放器的视频解码器配置失败.
	PLAYER_ERROR_AUDIO_DECODER_ERROR   , ///< 播放器的音频解码器配置失败.
	PLAYER_ERROR_SUB_DECODER_ERROR     , ///< 播放器的字幕解码器配置失败.
	PLAYER_ERROR_VIDEO_RENDER_ERROR    , ///< 播放器的视频输出配置失败,
	PLAYER_ERROR_AUDIO_RENDER_ERROR    , ///< 播放器的音频输出配置失败.
	PLAYER_ERROR_SUB_RENDER_ERROR      , ///< 播放器的字幕配置失败.
	PLAYER_ERROR_ACCESS_ERROR          , ///< 播放器不支持 URL 解析.
	PLAYER_ERROR_MUXER_ERROR           , ///< 播放器的复用器配置失败
	PLAYER_ERROR_DUMPER_ERROR          , ///< 播放器的录制器配置失败.
	PLAYER_ERROR_SOURCE_RESTART        , ///< 网络服务端出错,需要重启播放器.
	PLAYER_ERROR_URL_UNSUPPORT         , ///< 网络协议未注入(协议未找到).
	PLAYER_ERROR_MALLOC_FAILED         , ///< 内存不足（不管是什么情况下申请内存失败）
	PLAYER_ERROR_URL_OPEN_FAILED       , ///< url无法访问
	PLAYER_ERROR_NET_READ_FAILED       , ///< url下载、缓存数据失败或超时
	PLAYER_ERROR_CALL_MEDIA_SEEK       , ///< seek失败
	PLAYER_ERROR_NET_SERVER_ERROR      , ///< 服务器错误
	PLAYER_ERROR_CALL_MEDIA_SPEED      , ///< 快进、回放、失败
}PlayerError;

/**
 * 播放器跳转标识
 **/
typedef enum {
	SEEK_ORIGIN_SET     ,  ///< 跳转偏移量为设置的偏移量.
	SEEK_ORIGIN_CUR     ,  ///< 跳转偏移量为当前的偏移量.
	SEEK_ORIGIN_END     ,  ///< 跳转偏移量为结束之前的偏移量.
	SEEK_ORIGIN_PERCENT ,  ///< 跳转偏移量为设置百分比的偏移量.
} SeekFlag;

/**
 * 播放器流的类型
 **/
typedef enum {
	PLAYER_MEDIA_AUDIO      = (1 << 0),  ///< 音频流.
	PLAYER_MEDIA_VIDEO      = (1 << 1),  ///< 视频流.
	PLAYER_MEDIA_SUB        = (1 << 2),  ///< 字幕流.
	PLAYER_MEDIA_AUDIO_DESC = (1 << 3),  ///< 次路音频流.
	PLAYER_MEDIA_ALL        = 0xFFFFFFFF,///< 描述流的最大值.
} PlayerMediaContent;

/**
 * 播放器 stream 的 TS-BUFF 增加多路流的 pid 参数
 **/
typedef struct {
	int num;                                ///< 流的 pid 个数.
	unsigned short pid[GXSTREAM_MAX_TRACK]; ///< 流的 pid.
} GxStreamTrackAdd;

/**
 * 播放器 stream 的 TS-BUFF 删除多路流的 pid 参数
 **/
typedef struct {
	int num;                                ///< 流的 pid 个数.
	unsigned short pid[GXSTREAM_MAX_TRACK]; ///< 流的 pid.
} GxStreamTrackDel;

/**
 * 播放器 stream 的 TS-BUFF 多路流的 pid 参数
 **/
typedef struct {
	int num;                                ///< 流的 pid 个数.
	unsigned short pid[GXSTREAM_MAX_TRACK]; ///< 流的 pid.
} GxStreamTrackCache;

/**
 * 播放器状态参数
 **/
typedef struct {
	PlayerStatus      status;  ///< 播放器状态.
	PlayerError       error;   ///< 播放器错误码.
	AVCodecStatus     acodec;  ///< 播放器的音频解码器状态.
	AVCodecStatus     vcodec;  ///< 播放器的视频解码器状态.
	AVCodecSyncStatus async;   ///< 播放器的音频同步状态.
	AVCodecSyncStatus vsync;   ///< 播放器的视频同步状态.
} PlayerStatusInfo;

/**
 * 播放器时间参数
 **/
typedef struct {
	uint64_t current;  ///< 播放器当前播放时间.
	uint64_t totle;    ///< 播放器播放总时间.
	uint64_t seek_min; ///< 播放器最小跳转时间.
} PlayTimeInfo;

/**
 * MP3 的 ID3V1 标签
 **/
#pragma pack(1)
typedef struct {
	char                    Header[4+1];   ///<  标签头,唯一标识: "TAG", 3 Byte.
	char                    Title[30+1];   ///<  标题: 30 Byte.
	char                    Artist[30+1];  ///<  作者: 30 Byte.
	char                    Album[30+1];   ///<  专集: 30 Byte.
	char                    Year[4+1];     ///<  出品年代: 4 Byte.
	char                    Comment[28+1]; ///<  备注: 28 Byte.
	char                    Reserve[1];    ///<  保留: 1 Byte.
	char                    Track[1+1];    ///<  音轨: 1 Byte.
	char                    Genre[1+1];    ///<  类型: 1 Byte.
}PlayerID3V1;
#pragma pack()

/**
 * MP3 的 ID3V2 标签
 **/
#pragma pack(1)
typedef struct id3_v2
{
	char                    fName[4+1];    ///< 描述的标识字段.
	char*                   pFBody;        ///< 描述的内容.
	int                     size;          ///< 描述的内容大小.
	struct                  id3_v2 *pNext; ///< 下一个 ID3V2 标签.
}PlayerID3V2;
#pragma pack()

/**
 * MP3 的标签参数
 **/
typedef struct {
	PlayerID3V1*            v1;  ///< ID3V1 标签.
	PlayerID3V2*            v2;  ///< ID3V2 标签.
} PlayerID3Info;

/**
 * 视频流的分数
 **/
typedef struct video_rational {
	int num;                       ///< 分子.
	int den;                       ///< 分母.
} PlayerVideoRational;

/**
 * 视频流的视角比
 **/
typedef enum {
	VIDEO_ASPECTRATIO_NONE,     ///< 未知的视角比.
	VIDEO_ASPECTRATIO_1BY1,     ///< 1:1 的视角比.
	VIDEO_ASPECTRATIO_4BY3,     ///< 4:3 的视角比.
	VIDEO_ASPECTRATIO_14BY9,    ///< 14:9 的视角比.
	VIDEO_ASPECTRATIO_16BY9,    ///< 16:9 的视角比.
	VIDEO_ASPECTRATIO_221BY1,   ///< 221:1 的视角比.
	VIDEO_ASPECTRATIO_RESERVE   ///< 预留的视角比.
}PlayerVideoAspectRatio;

/**
 * 视频流的动态范围信息
 **/
typedef struct {
	VideoDynamicRangeType type; ///< 视频动态范围类型
	unsigned char dolby_flag;   ///< 杜比标示
} PlayerVideoDynamicRangeInfo;

/**
 * 视频流信息参数
 **/
typedef struct {
	char                        codec_id[PLAYER_TARCK_CODEC_LONG]  ;  ///< 视频编码格式描述
	char                        track_name[PLAYER_TARCK_NAME_LONG] ;  ///< 视频轨道名称描述
	uint32_t                    width   ;                             ///< 视频帧宽
	uint32_t                    height  ;                             ///< 视频帧高
	uint32_t                    bpp;                                  ///< 视频每像素的编码色深
	uint32_t                    bitrate ;                             ///< 视频的比特率
	uint32_t                    interlace;                            ///< 视频是否是隔行编码
	float                       fps     ;                             ///< 视频帧率
	PlayerVideoRational         dar;                                  ///< 视频播放分辨率的宽高比例.
	PlayerVideoRational         sar;                                  ///< 视频采集分辨率的宽高比例.
	PlayerVideoAspectRatio      ratio;                                ///< 视频视角比.
	VideoCodecType              codec_type;                           ///< 视频编码.
	PlayerVideoDynamicRangeInfo dr;                                   ///< 视频动态范围.
}PlayerVideoTrack;

/**
 * 音频流信息参数
 **/
typedef struct {
	uint32_t                id;                                  ///< 音频流 pid.
	char                    lang[PLAYER_TARCK_LANG_LONG];        ///< 音频语言类型描述.
	char                    codec_id[PLAYER_TARCK_CODEC_LONG]  ; ///< 音频编码格式描述.
	char                    track_name[PLAYER_TARCK_NAME_LONG] ; ///< 音频轨道名称描述.
	uint32_t                bitrate    ;                         ///< 音频比特率.
	uint32_t                samplerate ;                         ///< 音频采样率.
	uint32_t                channels   ;                         ///< 音频声道数.
	AudioCodecType          codec_type;                          ///< 音频编码.
	AudioDecriptorTrack     track_type;                          ///< 音频主/次路.
}PlayerAudioTrack;

/**
 * 多带宽信息参数
 **/
typedef struct {
	unsigned int bandwidth; // 节目带宽
	unsigned int acodec;    // 节目音频编码
	unsigned int vcodec;    // 节目视频编码
	unsigned int width;     // 节目 宽
	unsigned int height;    // 节目 高
	unsigned int frameRate; // 帧率
}PlayerMulitBandwidthTrack;


/**
 * 节目信息参数
 **/
typedef struct {
	char                    url[PLAYER_URL_LONG+1];               ///< 节目的原始 URL(如:url -H **** or url).
	uint64_t                file_size;                            ///< 节目流的文件大小.
	uint64_t                duration ;                            ///< 节目流的播放时长.
	uint32_t                bitrate;                              ///< 节目流的比特率.
	PlayerVideoTrack        video;                                ///< 节目的视频流信息.
	PlayerAudioTrack        audio[PLAYER_MAX_TRACK_AUDIO];        ///< 节目的音频流信息,最大支持 16 路音频.
	PlayerSubTrack          sub[PLAYER_MAX_TRACK_SUB];            ///< 节目的字幕流信息,最大支持 32 路字幕.

	PlayerMulitBandwidthTrack   bandwidth[GXPLAYER_MAX_BAND_WIDTH];   ///< 节目的带宽信息,描述于 hls/dash等相关的节目流.
	int                     audio_num;                            ///< 节目的音频流个数.
	int                     sub_num;                              ///< 节目的字幕流个数.
	int                     bandwidth_num;                        ///< 节目的不同带宽流的个数.
	int                     is_radio;                             ///< 节目是否是广播节目.
	int                     cur_sub_id;                           ///< 当前播放字幕流的索引.
	int                     cur_audio_id;                         ///< 当前播放音频流的索引.
	int                     cur_bandwidth_id;                     ///< 当前播放带宽流的索引.
	uint64_t                rec_filesize;                         ///< 录制的文件总大小.
	uint64_t                rec_duration;                         ///< 录制的文件总时长.
	int                     eof;                                  ///< 文件播放是否结束.
	int                     net_speed;                            ///< 网络节目下载的速率.
	int                     buf_percent;                          ///< 网络节目缓冲的百分比,相对于播放器暂停/恢复播放门限的比例.
	int                     cache_buf_size;                       ///< 网络节目实际缓冲大小.
	int                     cache_buf_percent;                    ///< 网络节目占缓冲实际大小的比例.
	int                     cache_back_size;                      ///< 网络节目缓冲用于回退的缓冲大小.
	int                     cache_back_percent;                   ///< 网络节目缓冲用于回退的缓冲比例.
	int                     buffering;                            ///< 网络节目是否正在缓冲数据.
	GxAvFrameStat           video_stat;                           ///< 当前播放视频流的帧信息.
	GxAvFrameStat           audio_stat;                           ///< 当前播放音频流的帧信息.
	unsigned int            switch_cost_timems;                   ///< 节目切台时长.
	int                     seekable;                             ///< 节目是否可跳转.
	int                     cache_buf_time;                       ///< 缓冲数据可用于播放预估时长.
	int                     esa_percent;                          ///< 音频解码 ESA 可播放的数据比例.
	int                     esv_percent;                          ///< 视频解码 ESV 可播放的数据比例.
	char                    mime_type[PLAYER_MIME_TYPE_LONG];     ///< 节目信息描述.
} PlayerProgInfo;

/**
 * 节目流信息参数: 一个文件包含多个节目，一个节目由视频,多个音频,多个字幕组成．
 **/
typedef struct {
	unsigned int              prog_count;                           ///< 节目个数.
	struct {
		unsigned int          prog_id;                              ///< 节目 id .
		unsigned int          video_stream_count;                   ///< 每个节目的视频流个数.
		unsigned int          audio_stream_count;                   ///< 每个节目的音频流个数.
		unsigned int          subtitle_stream_count;                ///< 每个节目的字幕流个数.
		struct {
			unsigned int      pid;                                  ///< 视频 pid .
			VideoCodecType    codec_type;                           ///< 视频编码.
			char              lang[PLAYER_TARCK_LANG_LONG];         ///< 视频语言类型描述.
		} video_stream;                                             ///< 视频流信息
		struct {
			unsigned int      pid;                                  ///< 音频 pid .
			AudioCodecType    codec_type;                           ///< 音频编码.
			char              lang[PLAYER_TARCK_LANG_LONG];         ///< 音频语言类型描述.
		} audio_stream[PLAYER_MAX_PROG_STREAM_COUNT];               ///< 音频流信息
		struct {
			unsigned int      pid;                                  ///< 字幕 pid , 有些字幕需要 major , minor 才能描述字幕流,例如: TELETEXT.
			unsigned short    major;                                ///< 字幕流的主路信息．
			unsigned short    minor;                                ///< 字幕流的次路信息．
			PlayerSubType     type;                                 ///< 字幕类型.
			SubtitleCodecType codec_type;                           ///< 字幕编码.
			char              lang[PLAYER_TARCK_LANG_LONG];         ///< 字幕语言类型描述.
		} subtitle_stream[PLAYER_MAX_PROG_STREAM_COUNT];            ///< 字幕流信息.
	} prog[PLAYER_MAX_PROG_COUNT];                                  ///< 节目信息.
} PlayerProgStreamInfo;

/**
 * 播放节目消耗时间参数．
 **/
typedef struct {
	unsigned int play_cost_ms;                                    ///< 开启播放到视频启动显示的时间
	unsigned int seek_cost_ms;                                    ///< 开启跳转到视频启动显示的时间
} PlayerProgCostTime;

/**
 * 节目流的带宽参数
 **/
typedef struct {
	int                     cur_id;                               ///< 当前播放流的带宽索引.
	int                     num;                                  ///< 播放流的带宽个数.
	unsigned int            value[GXPLAYER_MAX_BAND_WIDTH];       ///< 播放流的带宽值.
} PlayerBandwidth;

/**
 * 切换多带宽多音轨多字幕时Dash时切换的下标流(customer api sync master)．
 **/
typedef struct {
	int                     		video_id;  ///< 当前播放流的Video索引下标.
	int                     		audio_id;  ///< 当前播放流的Audio索引下标.
	PlayerSubPID 		sub_id;                ///< 当前播放流的Subtile索引下标.
	PlayerSubtitle* 	subdata;
}PlayerMpegDashSwitch;

/**
 * 音视频同步的信息参数
 **/
typedef struct {
	int                     apts;               ///< 音频播放时间戳,单位(ms).
	int                     vpts;               ///< 视频播放时间戳,单位(ms).
} PlayerSyncInfo;

/**
 * 网络节目流的信息参数
 **/
typedef struct {
	int                     seek_flag;             ///< 网络节目是否可跳转.support v1 v2.
	int                     eof;                   ///< 网络节目播放是否结束.support v1 v2.
	int                     buffering;             ///< 网络节目播放是否正在缓冲.support v1 v2.
	int                     net_speed;             ///< 网络节目下载速率.support v1 v2.
	int                     buf_percent;           ///< 网路节目下载缓冲比,相对于播放器暂停/恢复播放门限的比例.support v1 v2.
	int                     cache_buf_size;        ///< 网络节目实际缓冲大小.support v1 v2.
	int                     cache_buf_percent;     ///< 网络节目占缓冲实际大小的比例.support v1 v2.
	int                     cache_back_size;       ///< 网络节目缓冲用于回退的缓冲大小.V1:support V2:not use.
	int                     cache_back_percent;    ///< 网络节目缓冲用于回退的缓冲比例.V1:support V2:not use.
	int64_t                 restart_time;          ///< 网路节目重启播放的时间点.V1:support V2:not use.
	PlayerBandwidth         bandwidth;             ///< 网络节目的带宽信息.support v1 v2.
	int                     cache_buf_time;        ///< 网络节目缓冲数据可用于播放预估时长.support v1 v2.
} PlayerNetInfo;

/**
 * 播放器 stream 的 TS-BUFF 配置参数
 **/
typedef struct {
	int                     cache2mem;                    ///< 是否从缓存到内存.
	int                     cachesize;                    ///< 缓存的大小.
	unsigned char           cachedir[PLAYER_URL_LONG+1];  ///< 缓存到磁盘时存储的路径，cache2mem = 0 时才有效.
} PlayerDelayConfig;

/**
 * 解复用器过滤数据类型
 **/
typedef enum {
	DEMUX_PES_TYPE,  ///< PES 数据
	DEMUX_PSI_TYPE,  ///< PSI 数据
	DEMUX_TS_TYPE    ///< TS  数据
} GxDemuxerDataType;

/**
 * 播放器播放以及过滤回调参数
 **/
typedef struct{
#define PLAYER_FILTER_MAX 64                                                                         ///< 最大支持 64 个过滤器.
	int vpid;                                                                                        ///< 播放视频流的 pid.
	int apid;                                                                                        ///< 播放音频流的 pid.
	int apid1;                                                                                       ///< 播放次路音频流的 pid.
	struct {
		int pid;                                                                                     ///< 过滤器的 pid.
		GxDemuxerDataType type;                                                                      ///< 过滤器过滤数据的类型.
		int crc;                                                                                     ///< 过滤器是否 CRC 校验.
		int (*writeback)(int pid, GxDemuxerDataType type, unsigned char *buffer, unsigned int size); ///< 过滤器的回调函数.
	}callback[PLAYER_FILTER_MAX];                                                                    ///< 过滤器回调参数.
} GxDemuxerPlayTsExtInfo;

/**
 * 播放器播放配置参数
 **/
typedef struct {
	GxDemuxerPlayTsExtInfo tsfilter;  ///< 播放过滤信息.
	GxStreamTrackCache     tscache;   ///< 播放 TS-Cache 信息, 用于快速切台(不缓冲整个TP，只缓冲个别节目).
} PlayerPlayConfig;

/**
 * 播放器状态事件参数
 **/
typedef struct {
	char                          player[PLAYER_NAME_LONG+1]; ///< 播放器名称描述.
	char                          url[PLAYER_URL_LONG+1];     ///< 播放器播放的 URL.
	PlayerStatus                  status;                     ///< 播放器播放状态信息.
	int                           error;                      ///< 播放器是否异常.
} PlayerEventStatus;

/**
 * 播放器解码事件参数
 **/
typedef struct {
	char                          player[PLAYER_NAME_LONG+1]; ///< 播放器名称描述.
	char                          url[PLAYER_URL_LONG+1];     ///< 播放器播放的 URL.
	AVCodecStatus                 vcodec;                     ///< 播放器视频解码状态.
	AVCodecStatus                 acodec;                     ///< 播放器音频解码状态.
#define ACODEC (1 << 0)                                       ///< 音频解码状态切换标识.
#define VCODEC (1 << 1)                                       ///< 视频解码状态切换标识.
	unsigned int                  changed;                    ///< 播放器音视频解码状态变化.
} PlayerEventAVCodec;

/**
 * 播放器制式事件参数
 **/
typedef struct {
	VideoOutputMode               resolution;   ///< 播放器制式输出.
} PlayerEventResolution;

/**
 * 播放器快进快退速率事件参数
 **/
typedef struct {
	char                          player[PLAYER_NAME_LONG+1]; ///< 播放器名称描述.
	float                         speed;                      ///< 播放器快进快退速率.
} PlayerEventSpeed;

/**
 * 播放器录制缓冲溢出事件参数
 **/
typedef struct {
	char                          player[PLAYER_NAME_LONG+1]; ///< 播放器名称描述.
} PlayerEventRecordOverflow;

/**
 * 播放器视频动态范围变化事件参数
 **/
typedef struct {
	char                          player[PLAYER_NAME_LONG+1]; ///< 播放器名称描述.
	PlayerVideoDynamicRangeInfo   dr;                         ///< 视频动态范围.
} PlayerEventVideoDynamicRangeChanged;

/**
 * 播放器的播放信息参数
 **/
typedef struct {
	int64_t start;       ///< 播放的起始时间.
	int volume;          ///< 播放的音量.
	int audio_pid;       ///< 播放音频的 pid.
	int audio1_pid;      ///< 播放次路音频的 pid.
	int video_pid;       ///< 播放视频的 pid.
	int sub_pid;         ///< 播放字幕的 pid.
	int abort;           ///< 是否播放.
	PlayerWindow rect;   ///< 播放视频窗口信息.
} PlayerPlayInfo;

/**
 * 读取播放器的数据类型
 **/
typedef enum {
	PLAYER_READ_DUMPER,          ///< 读取播放器录制的数据.
	PLAYER_READ_VDECODER_SUBCC,  ///< 读取播放器 ATSC-CC 数据.
}PlayerReadType;

/**
 * \brief   打开一个播放器.
 * \details 1. 打开一个播放器后，Window 参数指定的区域会填充为黑色。Window 的范围必须在虚拟分辨率范围之内。
 * \details 2. 如果没有特殊需求，不需要调用这个接口，可以直接使用 GxPlayer_MediaPlay 播放节目，系统内部按需自动打开播放器。
 * \details 3. 如果调用了这个接口，必须通过 GxPlayer_Close 释放资源, GxPlayer_MediaStop 只会停止播放，不会完全释放资源。
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   window [in]     播放器播放窗口, 详见: gxdocref PlayerWindow.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_Open(const char* name, PlayerWindow* window);

/**
 * \brief   关闭一个播放器. 释放播放器的所有资源。
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_Close(const char* name);

/**
 * \brief   启动播放器播放一路节目.
 * \details 1. 播放会申请资源,需要通过 GxPlayer_MediaStop /GxPlayer_Close 释放资源.
 * \details 2. 切换节目无需调用 GxPlayer_MediaStop, 可以直接调用 GxPlayer_MediaPlay 播放新的节目.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   url    [in]     播放的 URL.
 * \param   start  [in]     播放起始时间. 适用于多媒体文件、IPTV(点播).
 * \param   volume [in]     播放音量值. 仅当 PLAYER_CONFIG_AUDIO_VOLUME_PRIV = 1 时才生效.
 * \param   window [in]     视频显示区域. 必须落在虚拟分辨率范围之内，仅当 window != NULL && windown->width != 0 && window->height != 0 时才生效. 详见: gxdocref PlayerWindow.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaPlay(const char* name, const char* url, int64_t start, int volume, PlayerWindow* window);

/**
 * \brief   停止播放器播放/录制的节目.
 * \details 1. 释放播放器申请资源.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaStop(const char* name);

/**
 * \brief   暂停播放器播放一路节目.
 * \details 1. 配置时移使能,暂停播放会自动进入时移模式. 后续调用 gxdocref GxPlayer_MediaStop 或 gxdocref GxPlayer_MediaPlay 会退出时移模式
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaPause(const char* name);

/**
 * \brief   恢复播放器播放一路节目.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaResume(const char* name);

/**
 * \brief   跳转播放器播放一路节目.
 * \details 1. gxdocref PlayerProgInfo 中 seekable 非 0 的节目才能跳转.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   time   [in]     播放器跳转时间.
 * \param   flag   [in]     播放器跳转标识,详见: gxdocref SeekFlag.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaSeek(const char* name, int64_t time, SeekFlag flag);

/**
 * \brief   快进快退播放器播放一路节目.
 * \details 1. gxdocref PlayerProgInfo 中 seekable 非 0 的节目才能快进.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   speed  [in]     播放器播放的速度. 取值范围 -128~-1/4, 1/4~128
 * \parblock
 * -  播放器快进快退一路节目:
 *  - speed > 1 - 快进播放.
 *  - speed = 1 - 正常播放.
 *  - speed < 1 - 快退播放.
 * \endparblock
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaSpeed(const char* name, float speed);

/**
 * \brief   播放器快速退出播放的节目.
 * \details 1. 仅仅是为了支持 IPTV 快速切换节目. 非 IPTV 节目请勿使用该接口。
 * \details 2. 退出播放不会释放资源,需要通过 GxPlayer_MediaStop 释放资源.
 * \details 3. 这是一个同步接口，如果当前节目 DNS 太慢, 会导致接口返回较慢，用户体验较差。
 * \details 4. 对 IPTV 切换节目速度没有极高要求的用户，不建议使用该接口。
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaExitPlay(const char* name);

/**
 * \brief   播放器等待至节目的播放出来.
 * \details 1. 退出播放不会释放资源,需要通过 GxPlayer_MediaStop 释放资源.
 * \details 2. 同步操作,节目播放出来,接口才返回.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
player_attribute_deprecated
extern status_t GxPlayer_MediaWaitPlay(const char* name);

/**
 * \brief   启动播放一路节目.
 * \details 1. 播放会申请资源,需要通过 GxPlayer_MediaStop 释放资源.
 * \details 2. 播放协议 stream 包括: DVB, FILE, IPTV 等.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   srcurl [in]     播放的 URL.
 * \param   info   [in]     播放信息,详见: gxdocref PlayerPlayInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
player_attribute_deprecated
extern status_t GxPlayer_MediaPlay2(const char* name, const char* srcurl, PlayerPlayInfo *info);

/**
 * \brief   音频播放校准.
 * \details 1. 在播放音频过程中,可以提前或延后播放音频.
 * \details 2. 使用场景: 音频和视频不同步流的微调.
 * \details 3. 如果在播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   delay  [in]     播放校准时长,单位 ms : delay > 0: 延后播放音频; delay < 0 : 提前播放音频.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaAudioDelayMs(const char *name, int delay);

/**
 * \brief   视频播放校准.
 * \details 1. 在播放视频过程中,可以提前或延后播放视频.
 * \details 2. 使用场景: 音频和视频不同步流的微调.
 * \details 3. 如果在播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   delay  [in]     播放校准时长,单位 ms : delay > 0: 延后播放视频; delay < 0 : 提前播放视频.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaVideoDelayMs(const char *name, int delay);

/**
 * \brief   调整播放器播放窗口.
 * \details 1. 在播放视频过程调整播放窗口.
 * \details 2. 如果在播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   window [in]     播放使用窗口,详见: gxdocref PlayerWindow.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaWindow(const char* name, PlayerWindow* window);

/**
 * \brief   放大播放器播放视频某一个区域.
 * \details 1. 在播放视频过程对指定区域进行放大.
 * \details 2. 如果在播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   Clip   [in]     区域信息,详见: gxdocref PlayerWindow.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaClip(const char* name, PlayerWindow* Clip);

/**
 * \brief   音频 PTS 校准.
 * \details 1. 在播放音频过程调整音频 PTS.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   timems [in]     校准时长,单位: Byte.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaAudioSync(const char* name, int timems);

/**
 * \brief   切换播放器播放的音频流.
 * \details 1. 使用场景: 切换音轨.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   pid    [in]     音频唯一标识 pid: DVB 节目为 pid 信息;FILE / IPTV 等其他播放为索引信息.
 * \param   type   [in]     音频编码标准.
 * \param   url    [in]     URL 信息,废弃参数.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaAudioSwitch(const char* name, int pid, AudioCodecType type, char* url);

/**
 * \brief   隐藏播放器播放的视频.
 * \details 1. 通过关闭 VPP 层实现.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaVideoHide(const char* name);

/**
 * \brief   显示播放器播放的视频.
 * \details 1. 通过开启 VPP 层实现.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaVideoShow(const char* name);

/**
 * \brief   清除播放器缓存的 ES、FB
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaFlush(const char* name);

/**
 * \brief   配置播放器播放信息.
 * \details 1. 播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   config [in]     配置参数,详见: gxdocref PlayerPlayConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaPlayConfig(const char* name, PlayerPlayConfig* config);

/**
 * \brief   配置播放器 stream 的 TS-BUFF.
 * \details 1. 播放器启动播放之前调用. 如果 name 指定的播放器不存在, 会自动创建一个播放器, 并保存相关参数, 为 GxPlayer_MediaPlay 做准备.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   config [in]     配置参数,详见: gxdocref PlayerDelayConfig.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaDelayConfig(const char* name, PlayerDelayConfig* config);

/**
 * \brief   获取播放器音视频解码器 ESV / ESA 可播放数据量.
 * \details 1. 音视频播放过程中调用.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   aDataLen [out]  音频解码器 ESA 数据量.
 * \param   vDataLen [out]  音频解码器 ESV 数据量.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetDataLength(const char* name, uint32_t* aDataLen, uint32_t* vDataLen);

/**
 * \brief   获取播放器播放时间信息.
 * \details 1. 音视频播放过程中调用.
 * \details 1. 接口返回失败,时间信息不可用.
 *
 * \param   name   [in]      播放器名称描述,唯一标识.
 * \param   info   [out]     时间信息,详见: gxdocref PlayTimeInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetTime(const char* name, PlayTimeInfo *info);

/**
 * \brief   获取播放器可播放总时长.
 * \details 1. 音视频播放过程中调用.
 * \details 1. 接口返回失败,时间信息不可用.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   duration [out]  时长信息.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetDuration(const char* name, uint64_t *duration);

/**
 * \brief   获取播放器当前播放进度.
 * \details 1. 音视频播放过程中调用.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   current  [out]  播放进度,百分数: 0 ~ 100.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetPercent(const char* name, uint8_t* current);

/**
 * \brief   通过 URL 获取播放器节目信息.
 * \details 1. 播放器未播放前使用,探测流的节目信息.
 * \details 2. 播放器播放过程使用,会双倍占用硬件资源.
 *
 * \param   url      [in]   URL 参数.
 * \param   info     [out]  节目信息,详见: gxdocref PlayerProgInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetProgInfoByUrl(const char* url, PlayerProgInfo* info);

/**
 * \brief   通过 URL 获取播放器节目流信息,节目流由多路节目组成.
 * \details 1. 播放器未播放前使用,探测流的节目信息.
 * \details 2. 播放器播放过程使用,会双倍占用硬件资源.
 * \details 3. 信息只由解复用解析得到，不能获取解码相关信息．
 *
 * \param   url      [in]   URL 参数.
 * \param   info     [out]  节目信息,详见: gxdocref PlayerProgStreamInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetProgStreamInfoByUrl(const char* url, PlayerProgStreamInfo* info);

/**
 * \brief   通过播放器名称获取播放器节目信息.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   info     [out]  节目信息,详见: gxdocref PlayerProgInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetProgInfoByName(const char* name, PlayerProgInfo* info);

/**
 * \brief   通过播放器名称获取 同步pts 信息.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   info     [out]  同步pts 信息,详见: gxdocref PlayerSyncInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetSyncInfo(const char* name, PlayerSyncInfo* info);

/**
 * \brief   通过播放器名称获取播放器节目流信息,节目流由多路节目组成.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 * \details 2. 信息只由解复用解析得到，不能获取解码相关信息．
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   info     [out]  节目信息,详见: gxdocref PlayerProgStreamInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetProgStreamInfoByName(const char* name, PlayerProgStreamInfo* info);

/**
 * \brief   通过播放器名称获取播放器播放或者跳转时间.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   costtime [out]  节目信息,详见: gxdocref PlayerProgCostTime.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetCostTimeByName(const char *name, PlayerProgCostTime *costtime);

/**
 * \brief   通过播放器名称获取当前播放的音频是否是dual mono.
 *
 * \param   name      [in]   播放器名称描述,唯一标识.
 * \param   dual_mono [out]  dual_mono 标志.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetDualMonoByName(const char* name, int *dual_mono);

/**
 * \brief   通过播放器名称获取 IPTV 流网络信息.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   info     [out]  网络信息,详见: gxdocref PlayerNetInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetNetInfo(const char* name, PlayerNetInfo* info);

/**
 * \brief   通过播放器名称获取播放器状态.
 * \details 1. 播放器播放过程中使用,播放前使用返回失败.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   info     [out]  播放器状态信息,详见: gxdocref PlayerStatusInfo.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaGetStatus(const char* name, PlayerStatusInfo* info);

/**
 * \brief   释放 MP3 的 ID3V1, ID3V2 资源.
 * \details 1. 释放的资源为 gxdocref GxPlayer_MediaGetID3Info 返回的句柄.
 *
 * \param   pID3     [in]   MP3 信息,详见: gxdocref PlayerID3Info.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaFreeID3Info(PlayerID3Info* pID3);

/**
 * \brief   申请并解析 MP3 的 ID3V1, ID3V2 信息.
 * \details 1. 通过 gxdocref GxPlayer_MediaFreeID3Info 释放 MP3 的 ID3V1, ID3V2 资源.
 *
 * \param   file [in]   URL 参数.
 * \retval  非NULL      成功,返回句柄.
 * \retval  NULL        失败.
 **/
extern PlayerID3Info* GxPlayer_MediaGetID3Info(const char* file);

/**
 * \brief   无缝切换iptv hls的多带宽流.
 * \details 1. 仅支援IPTV hls多带宽模式.
 * \details 2. 从hls m3u8中获取流的带宽信息, 通过带宽索引下标无缝(需要服务器支持才能做到无缝)切换带宽流.
 * \details 3. 切换过程,原清晰度的流播放至结束.
 * \details 4. 区别:(
 *                  GxPlayer_MediaSeamlessBandwidthSwitch: 客户定制,需服务器配合,码率立刻切换，视频会稍微静帧片刻;
 *                  GxPlayer_MediaBandwidthSwitch:通用的接口,并不需要服务器配合,切换时会延迟切换无法保证无缝切换,视频切换过来可能不那个时间点(与seek时精度有关)但视频保持流畅播放;无服务器的客户建议使用:GxPlayer_MediaBandwidthSwitch 接口.
 *                  )
 * \details 5. 在上层获取信息时,通过 GxPlayer_MediaGetProgInfoByName 获取整个信息,其中也会获取到其带宽信息.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   index    [in]   需要切换的带宽索引下标.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaSeamlessBandwidthSwitch(const char* name, unsigned int index);

/**
 * \brief   切换音视频播放带宽流后重新播放.
 * \details 1. IPTV V1 / V2 版本有效.
 * \details 2. 可以获取流的带宽信息, 通过带宽索引后从上一个切换时的时间点开始播放.
 * \details 3. 切换过程,原清晰度的流播放立刻停掉,切换到目标的清晰度.
 * \details 4. 区别:(
 *                  GxPlayer_MediaSeamlessBandwidthSwitch: 客户定制,需服务器配合,码率立刻切换，视频会稍微静帧片刻;
 *                  GxPlayer_MediaBandwidthSwitch:通用的接口,并不需要服务器配合,切换时会延迟切换无法保证无缝切换,视频切换过来可能不那个时间点(与seek时精度有关)但视频保持流畅播放;无服务器的客户建议使用:GxPlayer_MediaBandwidthSwitch 接口.
 *                  )
 * \details 5. 在上层获取信息时,通过 GxPlayer_MediaGetProgInfoByName 获取整个信息,其中也会获取到其带宽信息.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   index    [in]   带宽索引下标(第几个带宽下标).
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaBandwidthSwitch(const char* name, unsigned int index);

/**
 * \brief   切换全探测的多带宽dash流的带宽或者音轨或者字幕(customer api sync master).
 * \details 1. IPTV V2 版本有效.
 * \details 3. 无缝切换dash带宽或者音轨或者字幕.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   index    [in]   索引下标.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaSeamlessMpegDashSwitch(const char* name, PlayerMpegDashSwitch indata);

/**
 * \brief   播放器启动缓存除音视频外 N 个 PID 的数据
 * \details 1. 只适用于 TSBuff
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   track    [in]   增加的 stream 信息, 详见: gxdocref GxStreamTrackAdd.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaTrackAdd(const char* name, GxStreamTrackAdd* track);

/**
 * \brief   播放器停止缓存除音视频外 N 个 PID 的数据
 * \details 1. 只适用于 TSBuff
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   track    [in]   删除的 stream 信息, 详见: gxdocref GxStreamTrackDel.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaTrackDel(const char* name, GxStreamTrackDel* track);

/**
 * \brief   开启播放器次路播放的音频流.
 * \details 1. 使用场景: AD 流的开启.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   pid      [in]   次路音频流的 pid.
 * \param   type     [in]   次路音频流编码信息.
 * \param   dmxid    [in]   解复用器 ID, 参数废弃.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaAdAudioEnable(const char* name, int pid, AudioCodecType type, int dmxid);

/**
 * \brief   关闭播放器次路播放的音频流.
 * \details 1. 使用场景: AD 流的关闭,实际上次路音频正常解码.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaAdAudioDisable(const char* name);

/**
 * \brief   保留播放器音视频播放.
 * \details 1. 通过该接口,可以在播放过程关闭音频或视频.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   content  [in]   播放器音视频参数,详见: gxdocref PlayerMediaContent.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaSave(const char* name, PlayerMediaContent content);

/**
 * \brief   恢复播放器音视频播放.
 * \details 1. 通过该接口,可以在播放过程开启音频或视频.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \param   content  [in]   播放器音视频参数,详见: gxdocref PlayerMediaContent.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaRestore(const char* name, PlayerMediaContent content);

/**
 * \brief   停止播放器播放的节目.
 * \details 1. 停止正在播放节目,不停止录制,需要通过 GxPlayer_MediaStop 释放资源.
 *
 * \param   name     [in]   播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaStopPlay(const char* name);

/**
 * \brief   释放一个播放器的播放资源
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_MediaDestroyPlay(const char* name);

/**
 * \brief   读取播放器播放节目数据.
 *
 * \param   name   [in]  播放器名称描述,唯一标识.
 * \param   type   [in]  数据类型,详见: gxdocref PlayerReadType.
 * \param   buffer [in]  存放数据地址.
 * \param   len    [in]  存放数据长度.
 * \retval  >=0          读到的数据长度.
 * \retval  <0           读取失败.
 **/
extern int GxPlayer_MediaRead(const char* name, PlayerReadType type, uint8_t *buffer, int len);

/**
 * \brief   获取当的播放的PTS.
 *
 * \param   name   [in]     播放器名称描述,唯一标识.
 * \param   pts  [in]       获取解码器的PTS
 * \endparblock
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern int GxPlayer_MediaCurrentPts(const char *name, int32_t* pts);

/**
 * HEH功能 需要的播放状态的信息
 **/
typedef struct GxPlayerInfo {
	PlayerHEHStatus status;
	int file_format;
	char *url;
} GxPlayerInfo;

typedef void (*EventCallback)(void*, void*);

/**
 * HEH功能的结构体
 **/
typedef struct GxPlayerMediaEventNode {
	EventCallback callback;
	void *usr_data;
	struct GxPlayerMediaEventNode* next;
} GxPlayerMediaEventNode;

/**
 * \brief   注册HEH功能事件的回调函数.
 *
* \param   callback   [in]  上层接受事件的函数.
 * \retval  !NULL           返回heh的头节点的地址.
 * \retval  NULL            失败.
 **/
extern void* GxPlayer_RegisterEventHook(EventCallback callback, void *usr_data);

/**
 * \brief   注销需要注销HEH功能事件的回调函数
 *
 * \param   FreeNode   [in] 需注销HEH的地址.
 **/
extern void GxPlayer_UnregisterEventHook(void* FreeNode);

/**
 * \brief   HEH功能: 播放器状态发生变化,进行赋值，并触发勾子(回调函数),上报状态.
 *
 * \param   status   [in]      当前播放器的状态.
 * \param   file_format  [in]  当前播放的类型(file/dvb/iptv/other).
 * \param   srcurl  [in]       获取播放的url.
 **/
extern void GxPlayer_TriggerEvent(PlayerHEHStatus status, int file_format, const char *url);

/**
 * \brief   HEH功能: 播放器状态(播放成功/开始缓存/结束缓存/播放结束/播放失败)时触发(调用)GxPlayer_TriggerEvent函数.
 *
 * \param   status   [in]      当前播放器的状态.
 * \param   file_format  [in]  当前播放的类型(file/dvb/iptv/other).
 * \param   srcurl  [in]       获取播放的url.
 **/
extern void GxPlayer_EventStatus(PlayerStatus status, int file_format, char*srcurl);

__END_DECLS

#endif
/** @}*/
