#ifndef __GX_PLAYER_SYSTEM__
#define __GX_PLAYER_SYSTEM__

#include "gxplayer_module.h"
#include "gxavdev.h"
#include "gxdebug.h"
#include "gx_common.h"

#if 1
#include "module/config/gxconfig.h"
#else
#define GxBus_ConfigGetInt(key,buf,defval)  *(int*)buf = defval;
#define GxBus_ConfigGet(key,buf,size,defval)  strncpy(buf,defval,size);
#endif

typedef enum {
	PSYS_SYSTEM_VERSION,
	PSYS_VOUT_INTERFACE,
	PSYS_VOUT_USABLE_IFACE,
	PSYS_VOUT_RESOLUTION,
	PSYS_VOUT_AUTOCONFIG,
	PSYS_VOUT_AUTOADAPT,
	PSYS_VOUT_ASPECT,
	PSYS_VOUT_SCREEN,
	PSYS_VOUT_COLOR,
	PSYS_VOUT_CAPTURE,
	PSYS_VOUT_CLIP,
	PSYS_VOUT_VIEW,
	PSYS_VOUT_HIDE,
	PSYS_VOUT_SHOW,
	PSYS_VOUT_DEF,
	PSYS_VOUT_SCARTMODE,
	PSYS_VOUT_CLOSED,
	PSYS_VOUT_POWER,
	PSYS_VOUT_POWER2,
	PSYS_VOUT_CVBSVIDWPORT,
	PSYS_HDMI_ENCODING_OUT,
	PSYS_VDEC_HDR_ENABLE,
	PSYS_VDEC_CC_ENABLE,
	PSYS_VDEC_CC_DISPLAY,
	PSYS_VDEC_USERDATA_LENGTH,
	PSYS_VDEC_TIMEOUT,
	PSYS_VDEC_MOSAIC_DROP,
	PSYS_VDEC_MOSAIC_GATE,
	PSYS_VDEC_SYNC_FLAG,
	PSYS_VDEC_SYNC_TIMEMS,
	PSYS_VDEC_SYNC_HIGH_TIMEMS,
	PSYS_VDEC_ENABLE_PPZOOM,
	PSYS_VDEC_FPS_SOURCE,
	PSYS_VDEC_MAX_FB_SIZE,
	PSYS_VDEC_ENABLE_DEINTERLACE,
	PSYS_VDEC_EXTRA_FB,
	PSYS_VDEC_DEINTERLACE_MAX_WIDTH,
	PSYS_VDEC_DEINTERLACE_MAX_HEIGHT,
	PSYS_VDEC_PLAY_BYPASS,
	PSYS_VDEC_SPEED_MODE,

	PSYS_VDRC_MODE,

	PSYS_BACKGROUND,
	PSYS_STREAM_WIDTH,
	PSYS_STREAM_HEIGHT,
	PSYS_STREAM_STRIDE,

	PSYS_ENABLE_AC3,
	PSYS_FIREWALL_FLAG,
	PSYS_ENABLE_AAC,
	PSYS_ENABLE_DTS,

	PSYS_AUDIOCODEC_MASK,
	PSYS_VIDEOCODEC_MASK,

	PSYS_AOUT_MUTE,
	PSYS_AOUT_POWER_MUTE,
	PSYS_AOUT_VOLUME,
	PSYS_AOUT_INTERFACE,
	PSYS_AOUT_CONFIGPORT,
	PSYS_AOUT_TRACK,
	PSYS_AOUT_DESCRIPTOR_TRACK,
	PSYS_AOUT_PRIVATE,
	PSYS_AOUT_AC3MODE,
	PSYS_AOUT_AACMODE,
	PSYS_AOUT_DOWNMIX,
	PSYS_AOUT_SPDSRC,
	PSYS_AOUT_HDMISRC,
	PSYS_AOUT_SYNC_FLAG,
	PSYS_AOUT_SYNC_TIMEMS,
	PSYS_AOUT_SYNC_HIGH_TIMEMS,
	PSYS_AOUT_SPEED_MUTE,
	PSYS_AOUT_SET_PORT,
	PSYS_AOUT_TURN_PORT,
	PSYS_AOUT_SET_DAC_VOLUME,
	PSYS_AOUT_VOLUME_FACTOR,

	PSYS_ADEC_BOOST_VOLUME,
	PSYS_ADEC_DESCRIPTOR_BOOST_VOLUME,

	PSYS_RECORD_TSW_MEMSIZE,

	PSYS_PVR_SHIFT_ENABLE,
	PSYS_PVR_SHIFT_DMXID,
	PSYS_PVR_SHIFT_FILE,
	PSYS_PVR_VOLUME_SIZEMB,
	PSYS_PVR_VOLUME_MAXNUM,
	PSYS_PVR_VOLUME_FULLSTOP,
	PSYS_PVR_VOLUME_MAXTIMES,
	PSYS_PVR_RESERVE_SIZEMB,
	PSYS_PVR_PLAY_DEMUX_ID,
	PSYS_PVR_PLAYBACK_HWTS_SYNCMODE,
	PSYS_PVR_PLAYBACK_FORCE_HWTS,

	PSYS_FREEZE_ENABLE,
	PSYS_FREEZE_START,
	PSYS_FREEZE_STOP,

	PSYS_EVENT_REPORT,

	PSYS_CBK_FREEZE,
	PSYS_CBK_UNFREEZE,
	PSYS_CBK_EVENT_REPORT,
	PSYS_CBK_SEEK,
	PSYS_CBK_INTERRUPT,
	PSYS_CBK_STREAM_PORT,

	PSYS_INDEX_CACHE,
	PSYS_PACKET_CACHE,
	PSYS_HOLEMEM_SIZE,
	PSYS_RECORD_CACHE,
	PSYS_STREAM_BUFFER_SIZE,
	PSYS_PLAY_SOF_REPLAY,
	PSYS_NETWORK_CACHE,
	PSYS_NETWORK_SEEK_CACHE,
	PSYS_NETWORK_TIMEOUT,
	PSYS_NETWORK_RECVBUF,
	PSYS_NETWORK_HLS_MULTIPLE_LIST,
	PSYS_NETWORK_FFHLS_MAX_PLAYLIST,
	PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS,
	PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX,
	PSYS_NETWORK_IS_FALSE_LIVE,
	PSYS_NETWORK_HTTP_AUTO_ADAPABILITY,
	PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION,
	PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE,
	PSYS_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE,
	PSYS_NETWORK_RESOLUTION_WIDTH,
	PSYS_NETWORK_RESOLUTION_HEIGHT,
	PSYS_NETWORK_AUDIO_DELAY,
	PSYS_PLAY_ERROR_STATUS,
	PSYS_MAX_URL_SIZE,
	PSYS_MEDIA_MAX_STREAMS,
	PSYS_MPEGDASH_MAX_PERIODS,
	PSYS_DELAY_CACHE_DIR,
	PSYS_DELAY_CACHE_TO_MEM,
	PSYS_DELAY_CACHE_SIZE,
	PSYS_DELAY_CACHE_HW_SIZE,
	PSYS_DELAY_CACHE_HOLE_SIZE,
	PSYS_BufSizeESV,
	PSYS_BufSizeESA,
	PSYS_BufSizeESS,
	PSYS_BufSizeAC3,
	PSYS_BufSizeEAC3,
	PSYS_BufSizePCM,
	PSYS_BufSizeDTS,
	PSYS_SysMemHoleSize,
	PSYS_BufSizeAAC,

	PSYS_ERROR_STOP,
	PSYS_AUDIO_DMX_FULL_GATE,
	PSYS_AUDIO_ANTI_ERROR_CODE,
	PSYS_AUDIO_PURE_RECOVERY,
	PSYS_ENCRYPT_KEY,

	PSYS_ENABLE_SCTE,
	PSYS_AUDIO_DESCRIPTOR_VAILD,
	PSYS_AUDIO_DOLBY_CERTIFY,
	PSYS_AUDIO_SPEED_MODE,

	PSYS_DEBUG_NETWORK_STREAM,
	PSYS_DEBUG_STREAM_SPEED,
	PSYS_DEBUG_STREAM_PVR,
	PSYS_DEBUG_PACKET_SPEED,
	PSYS_DEBUG_DEMUX_FILL_DATA,
	PSYS_DEBUG_DEMUX_APTS,
	PSYS_DEBUG_DEMUX_VPTS,
	PSYS_DEBUG_SUBTITLE_SYNC,
	PSYS_DEBUG_ADEC_CONFIG,
	PSYS_DEBUG_ADEC_CONTENT,
	PSYS_DEBUG_AOUT_CONFIG,
	PSYS_DEBUG_AOUT_CONTENT,
	PSYS_DEBUG_RTP_PACKET,
	PSYS_DEBUG_AV_INFO,
	PSYS_MAX,
}GxPlayerSystemCtrlID;

typedef struct {
	int timeout;
	int mosaic_drop;
	int mosaic_gate;
	int cc_enable;
	int cc_display;
	int hdr_enable;
	int userdata_length;
	int sync_flag;
	int sync_low_timems;
	int sync_high_timems;
	int max_fb_size;
	int enable_pp_zoom;
	int enable_deinterlace;
	int deinterlace_max_width;
	int deinterlace_max_height;
	int fps_source;
	int extra_fb;
	int speed_mode;
}Videodec;

typedef struct {
	int select;
	struct {
		GxVideoOutProperty_Mode      mode;

		unsigned int brightness;
		unsigned int contrast;
		unsigned int saturation;
	}iface_info[6];
#define mode_hdmi    iface_info[5].mode
#define mode_yuv     iface_info[2].mode
#define mode_rca     iface_info[0].mode
#define mode_rca1    iface_info[1].mode
#define mode_scart   iface_info[3].mode
#define mode_svideo  iface_info[4].mode

#define b_hdmi       iface_info[5].brightness
#define b_yuv        iface_info[2].brightness
#define b_rca        iface_info[0].brightness
#define b_rca1       iface_info[1].brightness
#define b_scart      iface_info[3].brightness
#define b_svideo     iface_info[4].brightness

#define c_hdmi       iface_info[5].contrast
#define c_yuv        iface_info[2].contrast
#define c_rca        iface_info[0].contrast
#define c_rca1       iface_info[1].contrast
#define c_scart      iface_info[3].contrast
#define c_svideo     iface_info[4].contrast

#define s_hdmi       iface_info[5].saturation
#define s_yuv        iface_info[2].saturation
#define s_rca        iface_info[0].saturation
#define s_rca1       iface_info[1].saturation
#define s_scart      iface_info[3].saturation
#define s_svideo     iface_info[4].saturation

	int autoadapt;
	int autopal;
	int autontsc;
	int closed;
	int autoadapt1;
	int sharpness_en;
	int hue_en;
}VideoOut;

typedef struct {
	int mute;
	int volume;
	int volume_priv;
	int volume_table;
	int spdsource;
	int hdmisource;
	int interface;
	int track;
	int ac3mode;
	int aacmode;
	int downmix;
	int descriptor_track;
	int sync_flag;
	int sync_low_timems;
	int sync_high_timems;
	int speed_mute;
}AudioOut;

typedef struct {
	int  shift_enable;
	int  shift_hold;
	int  shift_dmxid;
	int  volume_sizemb;
	int  volume_maxnum;
	int  volume_fullstop;
	int  reserve_sizemb;
	int  volume_maxtimes;
	int  play_dmxid;
	int  playback_hwts_syncmode;
	int  playback_force_hwts;
	char shift_file[PLAYER_URL_LONG+1];
}Pvr;

typedef struct {
	int  cache2mem;
	int  cachesize;
	int  hwcachesize;
	int  memholesize;
	char cachedir[PLAYER_URL_LONG+1];
}Delay;

typedef struct {
	int enable;
	unsigned int fb_addr;
	unsigned int fb_size;
	PLAYER_FREEZE_FRAME   cbk_freeze;
	PLAYER_UNFREEZE_FRAME cbk_unfreeze;
}FreezeSwitch;

typedef struct {
	int network_stream;
	int stream_speed;
	int stream_pvr;
	int packet_speed;
	int demux_fill_data;
	int demux_apts;
	int demux_vpts;
	int sub_sync;
	int rtp_packet;
	int av_info;
}PlayerDebug;

typedef struct {
	unsigned int system_version;
	int stream_buffer_size;
	int play_sof_replay;
	int indexcache;
	int packetcache;
	int recordcache;
	int networkcache;
	int networkseekcache;
	int networktimeout;
	int networkrecvbuf;
	int play_error_status;
	int networkhlsmultiplelist;
	int networkffhlsmaxplaylist;
	int http_persistent_connections;
	int network_is_falselive;
	int network_default_bandwidth_index;
	int http_auto_adapability;
	int network_segment_func_is_full_detection;
	int network_screen_to_demux_pts_difference;
	int max_url_size;
	int media_max_streams;
	int mpegdash_max_periods;
	int errorstop;
	int autoplay;
	int enable_ac3;
	int firewall_flag;
	int enable_aac;
	int enable_dts;
	int esv_size;
	int esa_size;
	int ess_size;
	int pcm_size;
	int ac3_size;
	int eac3_size;
	int dts_size;
	int holemem_size;
	int sysmem_hole_size;
	int aac_size;
	int network_audio_delay;

	int acodec_mask;
	int vcodec_mask;
	int audio_dmx_full_gate;
	int audio_anti_error_code;
	int audio_pure_recovery;
	int hw_buffer_size;

	unsigned int audio_boost_volume;
	unsigned int audio_decriptor_boost_volume;

	int enable_scte;

	PlayerEtsEncrypt ets_encrypt;
	PlayerDebug      debug;
	struct {
		int width;
		int height;
	} net_resolution;

	Pvr           pvr;
	Delay         delay;
	Videodec      vdec;
	VideoOut      vout;
	int           aspect;
	DisplayScreen screen;
	PlayerWindow  view;
	PlayerWindow  clip;
	AudioOut      aout;
	FreezeSwitch  freeze[2];

	VideoDrcMode drc_mode;

	struct {
		unsigned int last_stream_width;
		unsigned int last_stream_height;
		unsigned int last_stream_stride;
	} background;

	PLAYER_EVENT_REPORT cbk_report;
	PLAYER_SEEK_CBK     cbk_seek;
	PLAYER_INTERRUPT_CBK cbk_interrupt;
	PlayerStreamPortCallback cbk_stream_port;
	CUSTOM_SSL_CHPHER_SUITE_CBK cbk_custom_ssl_cipher_suite;
}GxPlayerSystem;

typedef struct {
	int           capture;
	PlayerWindow  window;
}PlayerMPICSwitch;

typedef struct {
	PlayerEventID event;
	void*         args;
}PlayerEventReport;

typedef struct {
	int codec;
	int mode;
	int support;
} PlayerVaildAD;

typedef enum {
	SNAP_LAYER_OSD          ,
	SNAP_LAYER_VPP          ,
	SNAP_LAYER_SPP          ,
	SNAP_LAYER_BKG          ,
	SNAP_LAYER_MIX          ,
} PlayerLayer;

typedef struct {
	PlayerLayer             layer;
	int                     force;
} PlayerVideoShow;

typedef struct {
	PlayerLayer             layer;
	PlayerWindow            rect;
	void*                   surface;
} PlayerSnapshot;

extern int  GxPlayer_SystemInit(void);

extern void GxPlayer_SystemUninit(void);

extern int  GxPlayer_SystemSet(int id, void* args);

extern int  GxPlayer_SystemGet(int id, void* args);

extern int  GxPlayer_SystemFreezeFrameBufferAlloc(int id, unsigned int* addr, unsigned int* size);

extern int  GxPlayer_SystemFreezeFrameBufferFree(void);

extern void GxPlayer_HeapInit(void);

#endif

