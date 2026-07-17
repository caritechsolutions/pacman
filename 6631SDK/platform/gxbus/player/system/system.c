#include "gx_system.h"
#include "gx_device.h"
#include "gx_mediafilter.h"
#include "gx_mem.h"

#define VOUT_MASK_VAL1 (GX_VIDEO_OUTPUT_RCA1-1)
#define VOUT_MASK_BIT1 (8)
static GxPlayerSystem PSystem;

static int sys2dev[PSYS_MAX];

static void system_init(void)
{
	int ao_interface;
	int vo_interface;
	int vo_mode;

	memset(&PSystem, 0, sizeof(PSystem));

	GxDevice_Get(PDEV_SysVersion, &PSystem.system_version);
	GxDevice_Get(PDEV_AudioOutputDefInterface, &ao_interface);
	GxDevice_Get(PDEV_VideoOutputDefInterface, &vo_interface);
	GxDevice_Get(PDEV_VideoOutputDefConfig, &vo_mode);
	GxDevice_Get(PDEV_FreezeFrameSize, &PSystem.freeze[0].fb_size);
	PSystem.freeze[1].fb_size = PSystem.freeze[0].fb_size;
	GxDevice_Get(PDEV_IndexCacheSize, &PSystem.indexcache);
	GxDevice_Get(PDEV_PacketCacheSize, &PSystem.packetcache);
	GxDevice_Get(PDEV_RecordCacheSize, &PSystem.recordcache);
	GxDevice_Get(PDEV_VideoDeInterlace, &PSystem.vdec.enable_deinterlace);
	GxDevice_Get(PDEV_VideoPPZoom, &PSystem.vdec.enable_pp_zoom);
	GxDevice_Get(PDEV_EnableAc3,  &PSystem.enable_ac3);
	GxDevice_Get(PDEV_EnableAac,  &PSystem.enable_aac);
	GxDevice_Get(PDEV_EnableDts,  &PSystem.enable_dts);
	GxDevice_Get(PDEV_BufSizeESV, &PSystem.esv_size);
	GxDevice_Get(PDEV_BufSizeESA, &PSystem.esa_size);
	GxDevice_Get(PDEV_BufSizeESS, &PSystem.ess_size);
	GxDevice_Get(PDEV_BufSizeAC3, &PSystem.ac3_size);
	GxDevice_Get(PDEV_BufSizeEAC3, &PSystem.eac3_size);
	GxDevice_Get(PDEV_BufSizePCM, &PSystem.pcm_size);
	GxDevice_Get(PDEV_BufSizeDTS, &PSystem.dts_size);
	GxDevice_Get(PDEV_BufSizeAAC, &PSystem.aac_size);

	GxBus_ConfigGetInt(PLAYER_CONFIG_STREAM_BUFFER_SIZE      , &PSystem.stream_buffer_size, PLAY_STREAM_BUFFER_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_FLAG_SOF_REPLAY         , &PSystem.play_sof_replay,    PLAY_PLAY_SOF_REPLAY);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE  , &PSystem.networkcache,     PLAY_CACHE_SIZE_NET);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_CACHED_SEEK_SIZE, &PSystem.networkseekcache, PLAY_CACHE_SEEK_SIZE_NET);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_TIMEOUT         , &PSystem.networktimeout,   PLAY_NETWORK_TIMEOUT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_RESOLUTION_WIDTH   , &PSystem.net_resolution.width,   PLAY_NETWORK_RESOLUTION_WIDTH);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_RESOLUTION_HEIGHT  , &PSystem.net_resolution.height,   PLAY_NETWORK_RESOLUTION_HEIGHT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_AUDIO_DELAY     , &PSystem.network_audio_delay,   PLAY_NETWORK_AUDIO_DELAY);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_RECVBUF         , &PSystem.networkrecvbuf,   PLAY_NETWORK_RECVBUF);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_HLS_MULIPLE_LIST, &PSystem.networkhlsmultiplelist,   PLAY_NETWORK_HLS_MULTIPLE_LIST);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_FFHLS_MAX_PLAYLIST, &PSystem.networkffhlsmaxplaylist,  PLAY_NETWORK_FFHLS_MAX_PLAYLIST);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_HTTP_PERSISTENT_CONNECTIONS, &PSystem.http_persistent_connections,  PLAY_NETWORK_HTTP_PERSISTENT_CONNECTIONS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_DEFAULT_BANDWIDTH_INDEX, &PSystem.network_default_bandwidth_index,  PLAY_NETWORK_DEFAULT_BANDWIDTH_INDEX);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_IS_FALSE_LIVE   , &PSystem.network_is_falselive,  PLAY_NETWORK_IS_FALSE_LIVE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_HTTP_AUTO_ADAPABILITY   , &PSystem.http_auto_adapability,  PLAY_NETWORK_HTTP_AUTO_ADAPABILITY);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION   , &PSystem.network_segment_func_is_full_detection,  PLAY_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION);
	GxBus_ConfigGetInt(PLAYER_CONFIG_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE   , &PSystem.network_screen_to_demux_pts_difference,  PLAY_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_MAX_URL_SIZE   , &PSystem.max_url_size,  PLAY_DEFAULT_MAX_URL_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_MEDIA_MAX_STREAMS       , &PSystem.media_max_streams,        PLAY_MEDIA_MAX_STREAMS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_MPEGDASH_MAX_PERIODS    , &PSystem.mpegdash_max_periods,    PLAY_MPEGDASH_MAX_PERIODS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_FLAG_ERROR_STOP         , &PSystem.errorstop,        PLAY_ERROR_STOP);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_INTERFACE         , &PSystem.vout.select, vo_interface);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_HDMI     , (int32_t *)&PSystem.vout.mode_hdmi  , GX_VIDEO_OUTPUT_1080I_50HZ);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_YUV      , (int32_t *)&PSystem.vout.mode_yuv   , GX_VIDEO_OUTPUT_1080I_50HZ);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_RCA      , (int32_t *)&PSystem.vout.mode_rca   , GX_VIDEO_OUTPUT_PAL);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_RCA1     , (int32_t *)&PSystem.vout.mode_rca1  , GX_VIDEO_OUTPUT_PAL);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_SCART    , (int32_t *)&PSystem.vout.mode_scart , GX_VIDEO_OUTPUT_PAL);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_RESOLUTION_SVIDEO   , (int32_t *)&PSystem.vout.mode_svideo, GX_VIDEO_OUTPUT_PAL);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_HDMI     , (int *)&PSystem.vout.b_hdmi   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_YUV      , (int *)&PSystem.vout.b_yuv    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA      , (int *)&PSystem.vout.b_rca    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA1     , (int *)&PSystem.vout.b_rca1   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_SCART    , (int *)&PSystem.vout.b_scart  , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_BRIGHTNESS_SVIDEO   , (int *)&PSystem.vout.b_svideo , 50);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_HDMI     , (int *)&PSystem.vout.s_hdmi   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_YUV      , (int *)&PSystem.vout.s_yuv    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_RCA      , (int *)&PSystem.vout.s_rca    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_RCA1     , (int *)&PSystem.vout.s_rca1   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_SCART    , (int *)&PSystem.vout.s_scart  , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SATURATION_SVIDEO   , (int *)&PSystem.vout.s_svideo , 50);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_HDMI       , (int *)&PSystem.vout.c_hdmi   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_YUV        , (int *)&PSystem.vout.c_yuv    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_RCA        , (int *)&PSystem.vout.c_rca    , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_RCA1       , (int *)&PSystem.vout.c_rca1   , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_SCART      , (int *)&PSystem.vout.c_scart  , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_CONTRAST_SVIDEO     , (int *)&PSystem.vout.c_svideo , 50);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DRCMODE             , (int *)&PSystem.drc_mode, VIDEO_DRCMODE);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN    , &PSystem.screen.aspect, VIDEO_DISPLAY_SCREEN);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_ASPECT            , &PSystem.aspect, VIDEO_ASPECT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_AUTO_ADAPT        , &PSystem.vout.autoadapt, VIDEO_AUTO_ADAPT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_AUTO_ADAPT1       , &PSystem.vout.autoadapt1, VIDEO_AUTO_ADAPT1);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_AUTO_PAL          , &PSystem.vout.autopal, VIDEO_AUTO_PAL);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_AUTO_NTSC         , &PSystem.vout.autontsc, VIDEO_AUTO_NTSC);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SHARPNESS_EN      , &PSystem.vout.sharpness_en, VIDEO_SHARPNESS_EN);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_HUE_EN            , &PSystem.vout.hue_en,       VIDEO_HUE_EN);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SCREEN_XRES       , &PSystem.screen.xres, VIDEO_SCREEN_XRES);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SCREEN_YRES       , &PSystem.screen.yres, VIDEO_SCREEN_YRES);

	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FREEZE_SWITCH     , &PSystem.freeze[0].enable, PLAY_FREEZE_SWITCH);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_TIMEOUT       , &PSystem.vdec.timeout, PLAY_FREEZE_TIMEOUT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_HDR_ENABLE    , &PSystem.vdec.hdr_enable, VIDEO_HDR_ENABLE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_CC_ENABLE     , &PSystem.vdec.cc_enable,  VIDEO_CC_ENABLE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_CC_DISPLAY    , &PSystem.vdec.cc_display, VIDEO_CC_DISPLAY);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_USERDATA_LENGTH,  &PSystem.vdec.userdata_length,  VIDEO_USERDATA_LENGTH);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_MOSAIC_DROP   , &PSystem.vdec.mosaic_drop, VIDEO_MOSAIC_DROP_GATE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_MOSAIC_GATE   , &PSystem.vdec.mosaic_gate, VIDEO_MOSAIC_ERROR_GATE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_SYNC_FLAG     , &PSystem.vdec.sync_flag,   VIDEO_SYNC_FLAG);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_SYNC_TIMEMS   , &PSystem.vdec.sync_low_timems, VIDEO_SYNC_TIMEMS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DEC_SYNC_HIGH_TIMEMS, &PSystem.vdec.sync_high_timems, VIDEO_SYNC_HIGH_TIMEMS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_PP_MAX_WIDTH      , &PSystem.vdec.deinterlace_max_width, VIDEO_DEINTERLACE_WIDTH);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_PP_MAX_HEIGHT     , &PSystem.vdec.deinterlace_max_height, VIDEO_DEINTERLACE_HEIGHT);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FB_MAX_SIZE       , &PSystem.vdec.max_fb_size, VIDEO_FB_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_EXTRA_FB          , &PSystem.vdec.extra_fb, 0xFFFFFFFF);
	GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SPEED_MODE        , &PSystem.vdec.speed_mode, NORMAL_SPEED);

	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_ANTI_ERROR_CODE   , &PSystem.audio_anti_error_code, AUDIO_ANTI_ERROR_CODE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_DMX_FULL_GATE     , &PSystem.audio_dmx_full_gate, AUDIO_DMX_FULL_GATE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_INTERFACE         , &PSystem.aout.interface, ao_interface);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_SPDIF_SOURCE      , &PSystem.aout.spdsource, AUDIO_SPDIF_SOURCE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_HDMI_SOURCE       , &PSystem.aout.hdmisource, AUDIO_HDMI_SOURCE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOLUME            , &PSystem.aout.volume,   AUDIO_VOLUME_VALUE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOLUME_TABLE      , &PSystem.aout.volume_table, AUDIO_VOLTABLE_VALUE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOLUME_PRIV       , &PSystem.aout.volume_priv,  AUDIO_VOLUME_PRIVATE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_TRACK             , &PSystem.aout.track, AUDIO_TRACK);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_DOWNMIX           , &PSystem.aout.downmix, AUDIO_DOWNMIX);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AC3_MODE          , &PSystem.aout.ac3mode, AUDIO_AC3_DEFAULT_MODE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AAC_MODE          , &PSystem.aout.aacmode, AUDIO_AAC_DEFAULT_MODE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_SYNC_FLAG         , &PSystem.aout.sync_flag,    AUDIO_SYNC_FLAG);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_SYNC_TIMEMS       , &PSystem.aout.sync_low_timems,  AUDIO_SYNC_TIMEMS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_SYNC_HIGH_TIMEMS  , &PSystem.aout.sync_high_timems, AUDIO_SYNC_HIGH_TIMEMS);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_SPEED_MUTE        , &PSystem.aout.speed_mute,   AUDIO_SPEED_MUTE);

	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_BOOST_VOLUME            , (int*)&PSystem.audio_boost_volume,           AUDIO_BOOST_VOLUME_VALUE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_DESCRIPTOR_BOOST_VOLUME , (int*)&PSystem.audio_decriptor_boost_volume, AUDIO_BOOST_VOLUME_VALUE);

	GxBus_ConfigGetInt(PLAYER_CONFIG_RECORD_TSW_MEMSIZE      , &PSystem.hw_buffer_size, DEF_RECORD_TSW_MEMSIZE);

	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_TIME_SHIFT_FLAG     , &PSystem.pvr.shift_enable, PVR_TIME_SHIFT_ENABLE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_TIME_SHIFT_DMXID    , &PSystem.pvr.shift_dmxid, PVR_TIME_SHIFT_DMXID);
	GxBus_ConfigGet   (PLAYER_CONFIG_PVR_TIME_SHIFT_FILE     , PSystem.pvr.shift_file, PLAYER_URL_LONG, PVR_TIME_SHIFT_FILE);

	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_RECORD_VOLUME_SIZE  ,   &PSystem.pvr.volume_sizemb, PVR_VOLUME_SIZEMB);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_RECORD_VOLUME_MAX   ,   &PSystem.pvr.volume_maxnum, PVR_VOLUME_MAX_NB);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_RECORD_VOLUME_FULLSTOP, &PSystem.pvr.volume_fullstop, PVR_VOLUME_FULLSTOP);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_RECORD_RESERVE_SIZE ,   &PSystem.pvr.reserve_sizemb, PVR_RESERVE_SIZEMB);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_PLAYBACK_HWTS_SYNCMODE, &PSystem.pvr.playback_hwts_syncmode, PVR_PLAYBACK_HWTS_SYNCMODE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_PLAYBACK_FORCE_HWTS  ,  &PSystem.pvr.playback_force_hwts,    PVR_PLAYBACK_FORCE_HWTS);

	GxBus_ConfigGetInt(PLAYER_CONFIG_PVR_PLAY_DEMUX_ID       , &PSystem.pvr.play_dmxid, PVR_PLAY_DMXID);

	GxBus_ConfigGetInt(PLAYER_CONFIG_DELAY_CACHE_SIZE        , &PSystem.delay.cachesize, PLAY_DELAY_CACHE_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE     , &PSystem.delay.hwcachesize, PLAY_DELAY_CACHE_HW_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_DELAY_CACHE_TO_MEMORY   , &PSystem.delay.cache2mem, PLAY_DELAY_CACHE_TO_MEMORY);

	GxBus_ConfigGetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG      , &PSystem.autoplay, PLAY_AUTO_PALY);

	GxBus_ConfigGetInt(PLAYER_CONFIG_PLAY_RESERVED_MEMSIZE   , &PSystem.packetcache, PSystem.packetcache);
	GxBus_ConfigGetInt(PLAYER_CONFIG_PLAY_RESERVED_HOLESIZE  , &PSystem.holemem_size, PLAYER_HOLEMEM_SIZE);
	GxBus_ConfigGetInt(PLAYER_CONFIG_INDEX_RESERVED_MEMSIZE  , &PSystem.indexcache, PSystem.indexcache);
	GxBus_ConfigGetInt(PLAYER_CONFIG_RECORD_CACHED_MEMSIZE   , &PSystem.recordcache, PSystem.recordcache);

	GxBus_ConfigGetInt(PLAYER_CONFIG_SUTITLE_ENABLE_SCTE     , &PSystem.enable_scte, SUBTITLE_ENABLE_SCTE);

	sys2dev[PSYS_SYSTEM_VERSION    ] = -1  ;

	sys2dev[PSYS_VOUT_INTERFACE    ] = PDEV_VideoOutputInterface  ;
	sys2dev[PSYS_VOUT_USABLE_IFACE ] = PDEV_VideoOutputUsableIface;
	sys2dev[PSYS_VOUT_RESOLUTION   ] = PDEV_VideoOutputConfig     ;
	sys2dev[PSYS_VOUT_AUTOCONFIG   ] = PDEV_VideoAutoConfig       ;
	sys2dev[PSYS_VOUT_AUTOADAPT    ] = PDEV_VideoAutoAdapt        ;
	sys2dev[PSYS_VOUT_ASPECT       ] = PDEV_VideoAspect           ;
	sys2dev[PSYS_VDRC_MODE         ] = PDEV_VideoDrcMode          ;
	sys2dev[PSYS_VOUT_SCREEN       ] = PDEV_VideoDisplayScreen    ;
	sys2dev[PSYS_VOUT_COLOR        ] = PDEV_VideoColors           ;
	sys2dev[PSYS_VOUT_CAPTURE      ] = PDEV_VideoCapure           ;
	sys2dev[PSYS_VOUT_CVBSVIDWPORT ] = PDEV_CvbsViewport          ;
	sys2dev[PSYS_VDEC_PLAY_BYPASS  ] = PDEV_VideoPlayBypass       ;
	sys2dev[PSYS_VOUT_SCARTMODE    ] = PDEV_VideoOutScartMode     ;

	sys2dev[PSYS_VOUT_CLIP         ] = PDEV_VideoClip             ;
	sys2dev[PSYS_VOUT_VIEW         ] = PDEV_VideoView             ;
	sys2dev[PSYS_VOUT_HIDE         ] = PDEV_VideoHide             ;
	sys2dev[PSYS_VOUT_SHOW         ] = PDEV_VideoShow             ;
	sys2dev[PSYS_BACKGROUND        ] = PDEV_Background            ;
	sys2dev[PSYS_STREAM_WIDTH      ] = PDEV_StreamWidth           ;
	sys2dev[PSYS_STREAM_HEIGHT     ] = PDEV_StreamHeight          ;
	sys2dev[PSYS_STREAM_STRIDE     ] = PDEV_StreamStride          ;
	sys2dev[PSYS_VOUT_DEF          ] = PDEV_VideoOutputDef        ;
	sys2dev[PSYS_VOUT_CLOSED       ] = -1                         ;
	sys2dev[PSYS_VOUT_POWER        ] = PDEV_VideoOutputPower      ;
	sys2dev[PSYS_VOUT_POWER2       ] = PDEV_VideoOutputPower2     ;
	sys2dev[PSYS_HDMI_ENCODING_OUT ] = PDEV_HdmiEncodingOut       ;

	sys2dev[PSYS_VDEC_TIMEOUT      ] = -1                         ;
	sys2dev[PSYS_VDEC_MOSAIC_DROP  ] = -1                         ;
	sys2dev[PSYS_VDEC_HDR_ENABLE ] = -1                           ;
	sys2dev[PSYS_VDEC_CC_ENABLE ] = -1                            ;
	sys2dev[PSYS_VDEC_CC_DISPLAY] = -1                            ;
	sys2dev[PSYS_VDEC_USERDATA_LENGTH ] = -1                      ;
	sys2dev[PSYS_VDEC_MOSAIC_GATE  ] = PDEV_VideoMosaicDropGate   ;
	sys2dev[PSYS_VDEC_SYNC_FLAG    ] = -1                         ;
	sys2dev[PSYS_VDEC_SYNC_TIMEMS  ] = -1                         ;
	sys2dev[PSYS_VDEC_SYNC_HIGH_TIMEMS  ] = -1                    ;
	sys2dev[PSYS_VDEC_MAX_FB_SIZE  ] = -1                         ;
	sys2dev[PSYS_VDEC_ENABLE_PPZOOM] = -1                         ;
	sys2dev[PSYS_VDEC_FPS_SOURCE] = -1                            ;
	sys2dev[PSYS_VDEC_ENABLE_DEINTERLACE] = PDEV_VideoDeinterlaceEnable;
	sys2dev[PSYS_VDEC_EXTRA_FB] = -1                              ;
	sys2dev[PSYS_VDEC_DEINTERLACE_MAX_WIDTH] = -1                 ;
	sys2dev[PSYS_VDEC_DEINTERLACE_MAX_HEIGHT] = -1                ;
	sys2dev[PSYS_VDEC_SPEED_MODE] = -1                            ;

	sys2dev[PSYS_ENABLE_AC3        ] = -1                         ;
	sys2dev[PSYS_FIREWALL_FLAG     ] = -1                         ;
	sys2dev[PSYS_ENABLE_AAC        ] = -1                         ;
	sys2dev[PSYS_ENABLE_DTS        ] = -1                         ;
	sys2dev[PSYS_AUDIOCODEC_MASK   ] = -1                         ;
	sys2dev[PSYS_VIDEOCODEC_MASK   ] = -1                         ;
	sys2dev[PSYS_AUDIO_DMX_FULL_GATE]= -1                         ;
	sys2dev[PSYS_AUDIO_ANTI_ERROR_CODE] = -1                      ;
	sys2dev[PSYS_AUDIO_PURE_RECOVERY]   = -1                      ;
	sys2dev[PSYS_AUDIO_DOLBY_CERTIFY]    = PDEV_AudioDolbyCertify ;
	sys2dev[PSYS_AUDIO_DESCRIPTOR_VAILD] = PDEV_AudioDescriptorVaild;
	sys2dev[PSYS_AUDIO_SPEED_MODE]       = PDEV_AudioSpeedMode;

	sys2dev[PSYS_AOUT_MUTE         ] = PDEV_AudioMute             ;
	sys2dev[PSYS_AOUT_POWER_MUTE   ] = PDEV_AudioPowerMute        ;
	sys2dev[PSYS_AOUT_VOLUME       ] = PDEV_AudioVolume           ;
	sys2dev[PSYS_AOUT_INTERFACE    ] = PDEV_AudioOutputInterface  ;
	sys2dev[PSYS_AOUT_CONFIGPORT   ] = PDEV_AudioOutputConfigPort ;
	sys2dev[PSYS_AOUT_TRACK        ] = PDEV_AudioTrack            ;
	sys2dev[PSYS_AOUT_PRIVATE      ] = -1                         ;
	sys2dev[PSYS_AOUT_AC3MODE      ] = -1                         ;
	sys2dev[PSYS_AOUT_AACMODE      ] = -1                         ;
	sys2dev[PSYS_AOUT_DOWNMIX      ] = -1                         ;
	sys2dev[PSYS_AOUT_SPDSRC       ] = -1                         ;
	sys2dev[PSYS_AOUT_HDMISRC      ] = -1                         ;
	sys2dev[PSYS_AOUT_SYNC_FLAG    ] = -1                         ;
	sys2dev[PSYS_AOUT_SYNC_TIMEMS  ] = -1                         ;
	sys2dev[PSYS_AOUT_SYNC_HIGH_TIMEMS  ] = -1                    ;
	sys2dev[PSYS_AOUT_SPEED_MUTE   ] = -1                         ;
	sys2dev[PSYS_AOUT_DESCRIPTOR_TRACK] = PDEV_AudioDescriptorTrack;
	sys2dev[PSYS_AOUT_SET_PORT        ] = PDEV_AudioSetPort          ;
	sys2dev[PSYS_AOUT_TURN_PORT       ] = PDEV_AudioTurnPort         ;
	sys2dev[PSYS_AOUT_SET_DAC_VOLUME  ] = PDEV_AudioSetDACVolume     ;
	sys2dev[PSYS_AOUT_VOLUME_FACTOR   ] = PDEV_AudioVolumeFactor     ;

	sys2dev[PSYS_ADEC_BOOST_VOLUME           ] = PDEV_AudioBoostVolume          ;
	sys2dev[PSYS_ADEC_DESCRIPTOR_BOOST_VOLUME] = PDEV_AudioDescriptorBoostVolume;

	sys2dev[PSYS_RECORD_TSW_MEMSIZE  ] = -1                       ;

	sys2dev[PSYS_PVR_SHIFT_ENABLE  ] = -1                         ;
	sys2dev[PSYS_PVR_SHIFT_DMXID   ] = -1                         ;
	sys2dev[PSYS_PVR_SHIFT_FILE    ] = -1                         ;
	sys2dev[PSYS_PVR_VOLUME_SIZEMB ] = -1                         ;
	sys2dev[PSYS_PVR_VOLUME_MAXNUM ] = -1                         ;
	sys2dev[PSYS_PVR_VOLUME_FULLSTOP] = -1                        ;
	sys2dev[PSYS_PVR_RESERVE_SIZEMB]  = -1                        ;
	sys2dev[PSYS_PVR_VOLUME_MAXTIMES] = -1                        ;
	sys2dev[PSYS_PVR_PLAY_DEMUX_ID]   = -1                        ;
	sys2dev[PSYS_PVR_PLAYBACK_HWTS_SYNCMODE]    = -1              ;
	sys2dev[PSYS_PVR_PLAYBACK_FORCE_HWTS]    = -1                 ;

	sys2dev[PSYS_FREEZE_ENABLE     ] = -1                         ;
	sys2dev[PSYS_FREEZE_START      ] = PDEV_FreezeFrameStart      ;
	sys2dev[PSYS_FREEZE_STOP       ] = PDEV_FreezeFrameStop       ;

	sys2dev[PSYS_EVENT_REPORT      ] = -1                         ;

	sys2dev[PSYS_CBK_FREEZE        ] = -1                         ;
	sys2dev[PSYS_CBK_UNFREEZE      ] = -1                         ;
	sys2dev[PSYS_CBK_EVENT_REPORT  ] = -1                         ;
	sys2dev[PSYS_CBK_SEEK          ] = -1                         ;
	sys2dev[PSYS_CBK_INTERRUPT     ] = -1                         ;
	sys2dev[PSYS_CBK_STREAM_PORT   ] = -1                         ;

	sys2dev[PSYS_INDEX_CACHE       ] = -1                         ;
	sys2dev[PSYS_PACKET_CACHE      ] = -1                         ;
	sys2dev[PSYS_PLAY_SOF_REPLAY   ] = -1                         ;
	sys2dev[PSYS_HOLEMEM_SIZE      ] = -1                         ;
	sys2dev[PSYS_RECORD_CACHE      ] = -1                         ;
	sys2dev[PSYS_STREAM_BUFFER_SIZE] = -1                         ;
	sys2dev[PSYS_NETWORK_CACHE     ] = -1                         ;
	sys2dev[PSYS_NETWORK_SEEK_CACHE] = -1                         ;
	sys2dev[PSYS_NETWORK_TIMEOUT]    = -1                         ;
	sys2dev[PSYS_NETWORK_RECVBUF]    = -1                         ;
	sys2dev[PSYS_NETWORK_HLS_MULTIPLE_LIST]    = -1               ;
	sys2dev[PSYS_NETWORK_FFHLS_MAX_PLAYLIST]   = -1               ;
	sys2dev[PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS]   = -1      ;
	sys2dev[PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX]       = -1      ;
	sys2dev[PSYS_NETWORK_IS_FALSE_LIVE]   = -1                    ;
	sys2dev[PSYS_NETWORK_HTTP_AUTO_ADAPABILITY]   = -1            ;
	sys2dev[PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION]   = -1   ;
	sys2dev[PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE]    = -1         ;
	sys2dev[PSYS_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE]  = -1    ;
	sys2dev[PSYS_NETWORK_RESOLUTION_WIDTH]    = -1                ;
	sys2dev[PSYS_NETWORK_RESOLUTION_HEIGHT]    = -1               ;
	sys2dev[PSYS_NETWORK_AUDIO_DELAY]    = -1                     ;
	sys2dev[PSYS_PLAY_ERROR_STATUS]    = -1                       ;
	sys2dev[PSYS_MAX_URL_SIZE]    = -1                            ;
	sys2dev[PSYS_MEDIA_MAX_STREAMS]    = -1                       ;
	sys2dev[PSYS_MPEGDASH_MAX_PERIODS]    = -1                    ;
	sys2dev[PSYS_DELAY_CACHE_HW_SIZE] = -1                        ;
	sys2dev[PSYS_DELAY_CACHE_HOLE_SIZE] = -1                      ;
	sys2dev[PSYS_DELAY_CACHE_SIZE  ] = -1                         ;
	sys2dev[PSYS_DELAY_CACHE_DIR   ] = -1                         ;
	sys2dev[PSYS_DELAY_CACHE_TO_MEM] = -1                         ;
	sys2dev[PSYS_BufSizeESV        ] = -1                         ;
	sys2dev[PSYS_BufSizeESA        ] = -1                         ;
	sys2dev[PSYS_BufSizePCM        ] = -1                         ;
	sys2dev[PSYS_BufSizeAC3        ] = -1                         ;
	sys2dev[PSYS_BufSizeEAC3       ] = -1                         ;
	sys2dev[PSYS_BufSizeDTS        ] = -1                         ;
	sys2dev[PSYS_BufSizeAAC        ] = -1                         ;
	sys2dev[PSYS_BufSizeESS        ] = -1                         ;
	sys2dev[PSYS_SysMemHoleSize    ] = -1                         ;
	sys2dev[PSYS_ERROR_STOP        ] = -1                         ;
	sys2dev[PSYS_ENABLE_SCTE       ] = -1                         ;
	sys2dev[PSYS_ENCRYPT_KEY]           = PDEV_EncryptKey         ;
	sys2dev[PSYS_DEBUG_NETWORK_STREAM]  = -1                      ;
	sys2dev[PSYS_DEBUG_STREAM_SPEED]    = -1                      ;
	sys2dev[PSYS_DEBUG_RTP_PACKET]      = -1                      ;
	sys2dev[PSYS_DEBUG_STREAM_PVR]      = -1                      ;
	sys2dev[PSYS_DEBUG_PACKET_SPEED]    = -1                      ;
	sys2dev[PSYS_DEBUG_DEMUX_FILL_DATA] = -1                      ;
	sys2dev[PSYS_DEBUG_DEMUX_APTS]      = -1                      ;
	sys2dev[PSYS_DEBUG_DEMUX_VPTS]      = -1                      ;
	sys2dev[PSYS_DEBUG_DEMUX_VPTS]      = -1                      ;
	sys2dev[PSYS_DEBUG_SUBTITLE_SYNC]   = -1                      ;
	sys2dev[PSYS_DEBUG_AV_INFO]         = -1                      ;
	sys2dev[PSYS_DEBUG_ADEC_CONFIG]     = PDEV_AudioDebugAdecConfig  ;
	sys2dev[PSYS_DEBUG_ADEC_CONTENT]    = PDEV_AudioDebugAdecContent ;
	sys2dev[PSYS_DEBUG_AOUT_CONFIG]     = PDEV_AudioDebugAoutConfig  ;
	sys2dev[PSYS_DEBUG_AOUT_CONTENT]    = PDEV_AudioDebugAoutContent ;

	GxDevice_Set(PDEV_SysInit, &PSystem);
}

#define AV_MEM_CHECK(name, size) \
	if (size % 1024) \
		gxloge("\n[ERROR]: Buffer-%s, size = 0x%x, not 1K align!\n\n", name, size);

static int system_inited = 0;
int GxPlayer_SystemInit(void)
{
	if (system_inited != 0)
		return 0;

	if (GxDevice_Init() != GX_PLAYER_OK) {
		gxlogf("[ERROR] %s\n", __func__);
		return GX_PLAYER_ERROR;
	}

	system_init();

#ifndef NO_OS
	GxPlayer_DebugAdecContent(0);
	GxPlayer_DebugAoutContent(0);
	if (1) {
		unsigned int esvsize, esasize, pcmsize;

		GxPlayer_SystemGet(PSYS_BufSizeESV, &esvsize);
		AV_MEM_CHECK("ESV", esvsize);
		GxPlayer_SystemGet(PSYS_BufSizeESA, &esasize);
		AV_MEM_CHECK("ESA", esasize);
		GxPlayer_SystemGet(PSYS_BufSizePCM, &pcmsize);
		AV_MEM_CHECK("PCM", pcmsize);
		GxPlayer_SystemFreezeFrameBufferAlloc(0, NULL, NULL);
		av_memhole_free(av_memhole_malloc(esvsize, GX_PINFLAG_ESV), GX_PINFLAG_ESV);
		av_memhole_free(av_memhole_malloc(esasize, GX_PINFLAG_ESA), GX_PINFLAG_ESA);
		av_memhole_free(av_memhole_malloc(pcmsize, GX_PINFLAG_PCM), GX_PINFLAG_PCM);
	}
#endif

	system_inited = 1;

	return GX_PLAYER_OK;
}

void av_memhole_destroy(void);
void GxPlayer_SystemUninit(void)
{
	GxPlayer_SystemFreezeFrameBufferFree();
	av_memhole_destroy();
	GxDevice_Uninit();
	system_inited = 0;
	return;
}

int  GxPlayer_SystemSet(int id, void* arg)
{
	int devid;
	int ret = GX_PLAYER_ERROR;

	if(id < 0 || id > PSYS_MAX)
		return ret;

	devid = sys2dev[id];

	if(devid < 0)
	{
		CHECK_NULL(arg);

		switch(id)
		{
		case PSYS_BufSizeESA:
			PSystem.esa_size = *(int*)arg;
			break;
		case PSYS_SysMemHoleSize:
			PSystem.sysmem_hole_size = *(int*)arg;
			break;
		case PSYS_AOUT_PRIVATE:
			PSystem.aout.volume_priv = *(int*)arg;
			break;
		case PSYS_RECORD_TSW_MEMSIZE :
			PSystem.hw_buffer_size = *(int*)arg;
			break;
		case PSYS_PVR_SHIFT_ENABLE :
			PSystem.pvr.shift_enable = *(int*)arg;
			break;
		case PSYS_PVR_SHIFT_DMXID :
			PSystem.pvr.shift_dmxid = *(int*)arg;
			break;
		case PSYS_PVR_VOLUME_SIZEMB :
			PSystem.pvr.volume_sizemb = *(int*)arg;
			break;
		case PSYS_PVR_VOLUME_MAXNUM:
			PSystem.pvr.volume_maxnum = *(int*)arg;
			break;
		case PSYS_PVR_VOLUME_FULLSTOP:
			PSystem.pvr.volume_fullstop = *(int*)arg;
			break;
		case PSYS_PVR_RESERVE_SIZEMB:
			PSystem.pvr.reserve_sizemb = *(int*)arg;
			break;
		case PSYS_PVR_PLAY_DEMUX_ID:
			PSystem.pvr.play_dmxid = *(int*)arg;
			break;
		case PSYS_PVR_VOLUME_MAXTIMES:
			PSystem.pvr.volume_maxtimes = *(int*)arg;
			break;
		case PSYS_PVR_SHIFT_FILE :
			strncpy(PSystem.pvr.shift_file, arg, PLAYER_URL_LONG);
			break;
		case PSYS_PVR_PLAYBACK_HWTS_SYNCMODE:
			PSystem.pvr.playback_hwts_syncmode = *(int*)arg;
			break;
		case PSYS_PVR_PLAYBACK_FORCE_HWTS:
			PSystem.pvr.playback_force_hwts = *(int*)arg;
			break;
		case PSYS_DELAY_CACHE_DIR:
			strncpy(PSystem.delay.cachedir, arg, PLAYER_URL_LONG);
			break;
		case PSYS_DELAY_CACHE_TO_MEM:
			PSystem.delay.cache2mem = *(int*)arg;
			break;
		case PSYS_DELAY_CACHE_SIZE:
			PSystem.delay.cachesize = *(int*)arg;
			break;
		case PSYS_DELAY_CACHE_HW_SIZE:
			PSystem.delay.hwcachesize = *(int*)arg;
			break;
		case PSYS_DELAY_CACHE_HOLE_SIZE:
			PSystem.delay.memholesize = *(int*)arg;
			break;
		case PSYS_FREEZE_ENABLE:
			PSystem.freeze[0].enable = *(int*)arg;
			break;
		case PSYS_CBK_SEEK:
			PSystem.cbk_seek = (PLAYER_SEEK_CBK)arg;
			break;
		case PSYS_CBK_INTERRUPT:
			PSystem.cbk_interrupt = (PLAYER_INTERRUPT_CBK)arg;
			break;
		case PSYS_CBK_FREEZE:
			PSystem.freeze[0].cbk_freeze = (PLAYER_FREEZE_FRAME)arg;
			break;
		case PSYS_CBK_UNFREEZE:
			PSystem.freeze[0].cbk_unfreeze = (PLAYER_UNFREEZE_FRAME)arg;
			break;
		case PSYS_CBK_EVENT_REPORT:
			PSystem.cbk_report = (PLAYER_EVENT_REPORT)arg;
			break;
		case PSYS_CBK_STREAM_PORT:
			memcpy(&PSystem.cbk_stream_port, arg, sizeof(PlayerStreamPortCallback));
			break;
		case PSYS_EVENT_REPORT:
			if(PSystem.cbk_report) {
				PlayerEventReport* Report = arg;
				PSystem.cbk_report(Report->event, Report->args);
			}
			break;
		case PSYS_INDEX_CACHE:
			PSystem.indexcache = *(int*)arg;
			break;
		case PSYS_PACKET_CACHE:
			PSystem.packetcache = *(int*)arg;
			break;
		case PSYS_RECORD_CACHE:
			PSystem.recordcache = *(int*)arg;
			break;
		case PSYS_NETWORK_CACHE:
			PSystem.networkcache = *(int*)arg;
			break;
		case PSYS_NETWORK_SEEK_CACHE:
			PSystem.networkseekcache = *(int*)arg;
			break;
		case PSYS_NETWORK_TIMEOUT:
			PSystem.networktimeout = *(int*)arg;
			break;
		case PSYS_NETWORK_RECVBUF:
			PSystem.networkrecvbuf = *(int*)arg;
			break;
		case PSYS_PLAY_ERROR_STATUS:
			PSystem.play_error_status = *(int*)arg;
			break;
		case PSYS_NETWORK_HLS_MULTIPLE_LIST:
			PSystem.networkhlsmultiplelist = *(int*)arg;
			break;
		case PSYS_NETWORK_FFHLS_MAX_PLAYLIST:
			PSystem.networkffhlsmaxplaylist = *(int*)arg;
			break;
		case PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS:
			PSystem.http_persistent_connections = *(int*)arg;
			break;
		case PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX:
			PSystem.network_default_bandwidth_index = *(int*)arg;
			break;
		case PSYS_NETWORK_IS_FALSE_LIVE:
			PSystem.network_is_falselive = *(int*)arg;
			break;
		case PSYS_NETWORK_HTTP_AUTO_ADAPABILITY:
			PSystem.http_auto_adapability = *(int*)arg;
			break;
		case PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION:
			PSystem.network_segment_func_is_full_detection = *(int*)arg;
			break;
		case PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE:
			PSystem.cbk_custom_ssl_cipher_suite = (CUSTOM_SSL_CHPHER_SUITE_CBK)arg;
			break;
		case PSYS_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE:
			PSystem.network_screen_to_demux_pts_difference = *(int*)arg;
			break;
		case PSYS_MAX_URL_SIZE:
			PSystem.max_url_size = *(int*)arg;
			break;
		case PSYS_MEDIA_MAX_STREAMS:
			PSystem.media_max_streams = *(int*)arg;
			break;
		case PSYS_MPEGDASH_MAX_PERIODS:
			PSystem.mpegdash_max_periods = *(int*)arg;
			break;
		case PSYS_ERROR_STOP:
			PSystem.errorstop = *(int*)arg;
			break;
		case PSYS_AUDIO_DMX_FULL_GATE:
			PSystem.audio_dmx_full_gate = *(int*)arg;
			break;
		case PSYS_AUDIO_PURE_RECOVERY:
			PSystem.audio_pure_recovery = *(int*)arg;
			break;
		case PSYS_AOUT_AC3MODE:
			PSystem.aout.ac3mode = *(int*)arg;
			break;
		case PSYS_AOUT_AACMODE:
			PSystem.aout.aacmode = *(int*)arg;
			break;
		case PSYS_ENABLE_AC3:
			PSystem.enable_ac3 = *(int*)arg;
			break;
		case PSYS_AOUT_DOWNMIX:
			PSystem.aout.downmix = *(int*)arg;
			break;
		case PSYS_AOUT_SPDSRC:
			PSystem.aout.spdsource = *(int*)arg;
			break;
		case PSYS_AOUT_HDMISRC:
			PSystem.aout.hdmisource = *(int*)arg;
			break;
		case PSYS_AOUT_SYNC_FLAG:
			PSystem.aout.sync_flag  = *(int*)arg;
			break;
		case PSYS_AOUT_SYNC_TIMEMS:
			PSystem.aout.sync_low_timems  = *(int*)arg;
			break;
		case PSYS_AOUT_SYNC_HIGH_TIMEMS:
			PSystem.aout.sync_high_timems  = *(int*)arg;
			break;
		case PSYS_AOUT_SPEED_MUTE:
			PSystem.aout.speed_mute  = *(int*)arg;
			break;
		case PSYS_VOUT_CLOSED:
			PSystem.vout.closed = *(int*)arg;
			break;
		case PSYS_VDEC_TIMEOUT:
			PSystem.vdec.timeout= *(int*)arg;
			break;
		case PSYS_VDEC_MOSAIC_DROP:
			PSystem.vdec.mosaic_drop = *(int*)arg;
			break;
		case PSYS_VDEC_ENABLE_PPZOOM:
			PSystem.vdec.enable_pp_zoom = *(int*)arg;
			break;
		case PSYS_VDEC_FPS_SOURCE:
			PSystem.vdec.fps_source = *(int*)arg;
			break;
		case PSYS_VDEC_EXTRA_FB:
			PSystem.vdec.extra_fb = *(int*)arg;
			break;
		case PSYS_VDEC_MAX_FB_SIZE:
			PSystem.vdec.max_fb_size = *(int*)arg;
			break;
		case PSYS_VDEC_CC_ENABLE:
			PSystem.vdec.cc_enable = *(int*)arg;
			break;
		case PSYS_VDEC_CC_DISPLAY:
			PSystem.vdec.cc_display = *(int*)arg;
			break;
		case PSYS_VDEC_HDR_ENABLE:
			PSystem.vdec.hdr_enable = *(int*)arg;
			break;
		case PSYS_VDEC_USERDATA_LENGTH:
			PSystem.vdec.userdata_length= *(int*)arg;
			if (PSystem.vdec.userdata_length < PLAYER_USERDATA_BUF_SIZE)
				PSystem.vdec.userdata_length = PLAYER_USERDATA_BUF_SIZE;
			break;
		case PSYS_VDEC_SYNC_FLAG:
			PSystem.vdec.sync_flag = *(int*)arg;
			break;
		case PSYS_VDEC_SYNC_TIMEMS:
			PSystem.vdec.sync_low_timems = *(int*)arg;
			break;
		case PSYS_VDEC_SYNC_HIGH_TIMEMS:
			PSystem.vdec.sync_high_timems = *(int*)arg;
			break;
		case PSYS_VDEC_DEINTERLACE_MAX_WIDTH:
			PSystem.vdec.deinterlace_max_width = *(int*)arg;
			break;
		case PSYS_VDEC_DEINTERLACE_MAX_HEIGHT:
			PSystem.vdec.deinterlace_max_height = *(int*)arg;
			break;
		case PSYS_VDEC_SPEED_MODE:
			PSystem.vdec.speed_mode = *(int*)arg;
			break;
		case PSYS_AUDIOCODEC_MASK:
			PSystem.acodec_mask = *(int*)arg;
			break;
		case PSYS_VIDEOCODEC_MASK:
			PSystem.vcodec_mask = *(int*)arg;
			break;
		case PSYS_DEBUG_NETWORK_STREAM:
			PSystem.debug.network_stream = *(int*)arg;
			break;
		case PSYS_DEBUG_RTP_PACKET:
			PSystem.debug.rtp_packet = *(int*)arg;
			break;
		case PSYS_DEBUG_STREAM_SPEED:
			PSystem.debug.stream_speed = *(int*)arg;
			break;
		case PSYS_DEBUG_STREAM_PVR:
			PSystem.debug.stream_pvr = *(int*)arg;
			break;
		case PSYS_DEBUG_PACKET_SPEED:
			PSystem.debug.packet_speed = *(int*)arg;
			break;
		case PSYS_DEBUG_DEMUX_FILL_DATA:
			PSystem.debug.demux_fill_data = *(int*)arg;
			break;
		case PSYS_DEBUG_DEMUX_APTS:
			PSystem.debug.demux_apts = *(int*)arg;
			break;
		case PSYS_DEBUG_DEMUX_VPTS:
			PSystem.debug.demux_vpts = *(int*)arg;
			break;
		case PSYS_DEBUG_SUBTITLE_SYNC:
			PSystem.debug.sub_sync = *(int*)arg;
			break;
		case PSYS_DEBUG_AV_INFO:
			PSystem.debug.av_info = *(int*)arg;
			break;
		case PSYS_NETWORK_RESOLUTION_WIDTH:
			 PSystem.net_resolution.width = *(int*)arg;
			break;
		case PSYS_NETWORK_RESOLUTION_HEIGHT:
			 PSystem.net_resolution.height = *(int*)arg;
			 break;
		case PSYS_NETWORK_AUDIO_DELAY:
			PSystem.network_audio_delay = *(int*)arg;
			break;
		case PSYS_HOLEMEM_SIZE:
			PSystem.holemem_size = *(int*)arg;
			break;
		case PSYS_STREAM_BUFFER_SIZE:
			PSystem.stream_buffer_size = *(int*)arg;
			break;
		case PSYS_PLAY_SOF_REPLAY:
			PSystem.play_sof_replay = *(int*)arg;
			break;
		default:
			gxlogd("[PSYS] id = %d, Not Support Set !...\n",id);
			return ret;
		}

		return GX_PLAYER_OK;
	}

	return GxDevice_Set(devid, arg);
}

int  GxPlayer_SystemGet(int id, void* arg)
{
	int devid;
	int ret = GX_PLAYER_ERROR;

	if(id < 0 || id > PSYS_MAX)
		return ret;

	devid = sys2dev[id];

	if(devid < 0)
	{
		CHECK_NULL(arg);

		switch(id)
		{
		case PSYS_SYSTEM_VERSION:
			*(unsigned int*)arg = PSystem.system_version;
			break;
		case PSYS_SysMemHoleSize:
			*(int*)arg = PSystem.sysmem_hole_size;
			break;
		case PSYS_BufSizeESV:
			*(int*)arg = PSystem.esv_size;
			break;
		case PSYS_BufSizeESA:
			*(int*)arg = PSystem.esa_size;
			break;
		case PSYS_BufSizeESS:
			*(int*)arg = PSystem.ess_size;
			break;
		case PSYS_BufSizeAC3:
			*(int*)arg = PSystem.ac3_size;
			break;
		case PSYS_BufSizeEAC3:
			*(int*)arg = PSystem.eac3_size;
			break;
		case PSYS_BufSizeDTS:
			*(int*)arg = PSystem.dts_size;
			break;
		case PSYS_BufSizeAAC:
			*(int*)arg = PSystem.aac_size;
			break;
		case PSYS_BufSizePCM:
			*(int*)arg = PSystem.pcm_size;
			break;
		case PSYS_AOUT_PRIVATE:
			*(int*)arg = PSystem.aout.volume_priv;
			break;
		case PSYS_RECORD_TSW_MEMSIZE :
			*(int*)arg = PSystem.hw_buffer_size;
			break;
		case PSYS_PVR_SHIFT_ENABLE :
			*(int*)arg = PSystem.pvr.shift_enable;
			break;
		case PSYS_PVR_SHIFT_DMXID :
			*(int*)arg = PSystem.pvr.shift_dmxid;
			break;
		case PSYS_PVR_VOLUME_SIZEMB :
			*(int*)arg = PSystem.pvr.volume_sizemb;
			break;
		case PSYS_PVR_VOLUME_MAXNUM:
			*(int*)arg = PSystem.pvr.volume_maxnum;
			break;
		case PSYS_PVR_VOLUME_FULLSTOP:
			*(int*)arg = PSystem.pvr.volume_fullstop;
			break;
		case PSYS_PVR_RESERVE_SIZEMB:
			*(int*)arg = PSystem.pvr.reserve_sizemb;
			break;
		case PSYS_PVR_PLAY_DEMUX_ID:
			*(int*)arg = PSystem.pvr.play_dmxid;
			break;
		case PSYS_PVR_VOLUME_MAXTIMES:
			*(int*)arg = PSystem.pvr.volume_maxtimes;
			break;
		case PSYS_PVR_SHIFT_FILE :
			*(char**)arg = PSystem.pvr.shift_file;
			break;
		case PSYS_PVR_PLAYBACK_HWTS_SYNCMODE:
			*(int*)arg = PSystem.pvr.playback_hwts_syncmode;
			break;
		case PSYS_PVR_PLAYBACK_FORCE_HWTS:
			*(int*)arg = PSystem.pvr.playback_force_hwts;
			break;
		case PSYS_DELAY_CACHE_DIR:
			*(char**)arg = PSystem.delay.cachedir;
			break;
		case PSYS_DELAY_CACHE_TO_MEM:
			*(int*)arg = PSystem.delay.cache2mem;
			break;
		case PSYS_DELAY_CACHE_SIZE:
			GxBus_ConfigGetInt(PLAYER_CONFIG_DELAY_CACHE_SIZE, &PSystem.delay.cachesize, PLAY_DELAY_CACHE_SIZE);
			*(int*)arg = PSystem.delay.cachesize;
			break;
		case PSYS_DELAY_CACHE_HW_SIZE:
			GxBus_ConfigGetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE, &PSystem.delay.hwcachesize, PLAY_DELAY_CACHE_HW_SIZE);
			*(int*)arg = PSystem.delay.hwcachesize;
			break;
		case PSYS_DELAY_CACHE_HOLE_SIZE:
			*(int*)arg = PSystem.delay.memholesize;
			break;
		case PSYS_FREEZE_ENABLE:
			*(int*)arg = PSystem.freeze[0].enable;
			break;
		case PSYS_CBK_SEEK:
			*(PLAYER_SEEK_CBK*)arg = PSystem.cbk_seek;
			break;
		case PSYS_CBK_INTERRUPT:
			*(PLAYER_INTERRUPT_CBK*)arg = PSystem.cbk_interrupt;
			break;
		case PSYS_CBK_FREEZE:
			*(PLAYER_FREEZE_FRAME*)arg = PSystem.freeze[0].cbk_freeze;
			break;
		case PSYS_CBK_UNFREEZE:
			*(PLAYER_UNFREEZE_FRAME*)arg = PSystem.freeze[0].cbk_unfreeze;
			break;
		case PSYS_CBK_EVENT_REPORT:
			*(PLAYER_EVENT_REPORT*)arg = PSystem.cbk_report;
			break;
		case PSYS_CBK_STREAM_PORT:
			if ( PSystem.cbk_stream_port.Init &&
					PSystem.cbk_stream_port.Uninit &&
					PSystem.cbk_stream_port.Exit &&
					PSystem.cbk_stream_port.NewPacket &&
					PSystem.cbk_stream_port.DelPacket &&
					PSystem.cbk_stream_port.GetPacket &&
					PSystem.cbk_stream_port.PutPacket)
			memcpy(arg, &PSystem.cbk_stream_port, sizeof(PlayerStreamPortCallback));
			break;
			break;
		case PSYS_INDEX_CACHE:
			*(int*)arg = PSystem.indexcache;
			break;
		case PSYS_PACKET_CACHE:
			*(int*)arg = PSystem.packetcache;
			break;
		case PSYS_RECORD_CACHE:
			*(int*)arg = PSystem.recordcache;
			break;
		case PSYS_NETWORK_CACHE:
			*(int*)arg = PSystem.networkcache;
			break;
		case PSYS_NETWORK_SEEK_CACHE:
			*(int*)arg = PSystem.networkseekcache;
			break;
		case PSYS_NETWORK_TIMEOUT:
			*(int*)arg = PSystem.networktimeout;
			break;
		case PSYS_NETWORK_RECVBUF:
			*(int*)arg = PSystem.networkrecvbuf;
			break;
		case PSYS_NETWORK_HLS_MULTIPLE_LIST:
			*(int*)arg = PSystem.networkhlsmultiplelist;
			break;
		case PSYS_NETWORK_FFHLS_MAX_PLAYLIST:
			*(int*)arg = PSystem.networkffhlsmaxplaylist;
			break;
		case PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS:
			*(int*)arg = PSystem.http_persistent_connections;
			break;
		case PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX:
			*(int*)arg = PSystem.network_default_bandwidth_index;
			break;
		case PSYS_NETWORK_IS_FALSE_LIVE:
			*(int*)arg = PSystem.network_is_falselive;
			break;
		case PSYS_NETWORK_HTTP_AUTO_ADAPABILITY:
			*(int*)arg = PSystem.http_auto_adapability;
			break;
		case PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION:
			*(int*)arg = PSystem.network_segment_func_is_full_detection;
			break;
		case PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE:
			*(CUSTOM_SSL_CHPHER_SUITE_CBK*)arg = PSystem.cbk_custom_ssl_cipher_suite;
			break;
		case PSYS_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE:
			*(int*)arg = PSystem.network_screen_to_demux_pts_difference;
			break;
		case PSYS_PLAY_ERROR_STATUS:
			*(int*)arg = PSystem.play_error_status;
			break;
		case PSYS_MAX_URL_SIZE:
			*(int*)arg = PSystem.max_url_size;
			break;
		case PSYS_MEDIA_MAX_STREAMS:
			*(int*)arg = PSystem.media_max_streams;
			break;
		case PSYS_MPEGDASH_MAX_PERIODS:
			*(int*)arg = PSystem.mpegdash_max_periods;
			break;
		case PSYS_ERROR_STOP:
			*(int*)arg = PSystem.errorstop;
			break;
		case PSYS_AUDIO_DMX_FULL_GATE:
			*(int*)arg = PSystem.audio_dmx_full_gate;
			break;
		case PSYS_AUDIO_PURE_RECOVERY:
			*(int*)arg = PSystem.audio_pure_recovery;
			break;
		case PSYS_AUDIO_ANTI_ERROR_CODE:
			*(int*)arg = PSystem.audio_anti_error_code;
			break;
		case PSYS_AOUT_AC3MODE:
			*(int*)arg = PSystem.aout.ac3mode;
			break;
		case PSYS_AOUT_AACMODE:
			*(int*)arg = PSystem.aout.aacmode;
			break;
		case PSYS_ENABLE_AC3:
			*(int*)arg = PSystem.enable_ac3;
			break;
		case PSYS_ENABLE_AAC:
			*(int*)arg = PSystem.enable_aac;
			break;
		case PSYS_ENABLE_DTS:
			*(int*)arg = PSystem.enable_dts;
			break;
		case PSYS_FIREWALL_FLAG:
			*(int*)arg = PSystem.firewall_flag;
			break;
		case PSYS_AOUT_DOWNMIX:
			*(int*)arg = PSystem.aout.downmix;
			break;
		case PSYS_AOUT_SPDSRC:
			*(int*)arg = PSystem.aout.spdsource;
			break;
		case PSYS_AOUT_HDMISRC:
			*(int*)arg = PSystem.aout.hdmisource;
			break;
		case PSYS_AOUT_SYNC_FLAG:
			*(int*)arg = PSystem.aout.sync_flag;
			break;
		case PSYS_AOUT_SYNC_TIMEMS:
			*(int*)arg = PSystem.aout.sync_low_timems;
			break;
		case PSYS_AOUT_SYNC_HIGH_TIMEMS:
			*(int*)arg = PSystem.aout.sync_high_timems;
			break;
		case PSYS_AOUT_SPEED_MUTE:
			*(int*)arg = PSystem.aout.speed_mute;
			break;
		case PSYS_VOUT_CLOSED:
			*(int*)arg = PSystem.vout.closed;
			break;
		case PSYS_VDEC_TIMEOUT:
			*(int*)arg = PSystem.vdec.timeout;
			break;
		case PSYS_VDEC_MOSAIC_DROP:
			*(int*)arg = PSystem.vdec.mosaic_drop;
			break;
		case PSYS_VDEC_ENABLE_PPZOOM:
			*(int*)arg = PSystem.vdec.enable_pp_zoom;
			break;
		case PSYS_VDEC_FPS_SOURCE:
			*(int*)arg = PSystem.vdec.fps_source;
			break;
		case PSYS_VDEC_EXTRA_FB:
			*(int*)arg = PSystem.vdec.extra_fb;
			break;
		case PSYS_VDEC_MAX_FB_SIZE:
			GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FB_MAX_SIZE, &PSystem.vdec.max_fb_size, VIDEO_FB_SIZE);
			*(int*)arg = PSystem.vdec.max_fb_size;
			break;
		case PSYS_VDEC_CC_ENABLE:
			*(int*)arg = PSystem.vdec.cc_enable;
			break;
		case PSYS_VDEC_CC_DISPLAY:
			*(int*)arg =PSystem.vdec.cc_display;
			break;
		case PSYS_VDEC_HDR_ENABLE:
			*(int*)arg = PSystem.vdec.hdr_enable;
			break;
		case PSYS_VDEC_USERDATA_LENGTH:
			if (PSystem.vdec.userdata_length < PLAYER_USERDATA_BUF_SIZE)
				PSystem.vdec.userdata_length = PLAYER_USERDATA_BUF_SIZE;
			*(int*)arg = (PSystem.vdec.userdata_length > PLAYER_USERDATA_BUF_SIZE ? PLAYER_USERDATA_BUF_SIZE : PSystem.vdec.userdata_length);
			break;
		case PSYS_VDEC_SYNC_FLAG:
			*(int*)arg = PSystem.vdec.sync_flag;
			break;
		case PSYS_VDEC_SYNC_TIMEMS:
			*(int*)arg = PSystem.vdec.sync_low_timems;
			break;
		case PSYS_VDEC_SYNC_HIGH_TIMEMS:
			*(int*)arg = PSystem.vdec.sync_high_timems;
			break;
		case PSYS_VDEC_DEINTERLACE_MAX_WIDTH:
			*(int*)arg = PSystem.vdec.deinterlace_max_width;
			break;
		case PSYS_VDEC_DEINTERLACE_MAX_HEIGHT:
			*(int*)arg = PSystem.vdec.deinterlace_max_height;
			break;
		case PSYS_VDEC_SPEED_MODE:
			*(int*)arg = PSystem.vdec.speed_mode;
			break;
		case PSYS_AUDIOCODEC_MASK:
			*(int*)arg = PSystem.acodec_mask;
			break;
		case PSYS_VIDEOCODEC_MASK:
			*(int*)arg = PSystem.vcodec_mask;
			break;
		case PSYS_ENABLE_SCTE:
			*(int*)arg = PSystem.enable_scte;
			break;
		case PSYS_DEBUG_NETWORK_STREAM:
			*(int*)arg = PSystem.debug.network_stream;
			break;
		case PSYS_DEBUG_RTP_PACKET:
			*(int*)arg = PSystem.debug.rtp_packet;
			break;
		case PSYS_DEBUG_STREAM_SPEED:
			*(int*)arg = PSystem.debug.stream_speed;
			break;
		case PSYS_DEBUG_STREAM_PVR:
			*(int*)arg = PSystem.debug.stream_pvr;
			break;
		case PSYS_DEBUG_PACKET_SPEED:
			*(int*)arg = PSystem.debug.packet_speed;
			break;
		case PSYS_DEBUG_DEMUX_FILL_DATA:
			*(int*)arg = PSystem.debug.demux_fill_data;
			break;
		case PSYS_DEBUG_DEMUX_APTS:
			*(int*)arg = PSystem.debug.demux_apts;
			break;
		case PSYS_DEBUG_DEMUX_VPTS:
			*(int*)arg = PSystem.debug.demux_vpts;
			break;
		case PSYS_DEBUG_SUBTITLE_SYNC:
			*(int*)arg = PSystem.debug.sub_sync;
			break;
		case PSYS_DEBUG_AV_INFO:
			*(int*)arg = PSystem.debug.av_info;
			break;
		case PSYS_NETWORK_RESOLUTION_WIDTH:
			*(int*)arg = PSystem.net_resolution.width;
			break;
		case PSYS_NETWORK_RESOLUTION_HEIGHT:
			*(int*)arg = PSystem.net_resolution.height;
			break;
		case PSYS_HOLEMEM_SIZE:
			*(int*)arg = PSystem.holemem_size;
			break;
		case PSYS_STREAM_BUFFER_SIZE:
			*(int*)arg = PSystem.stream_buffer_size;
			break;
		case PSYS_PLAY_SOF_REPLAY:
			*(int*)arg = PSystem.play_sof_replay;
			break;
		case PSYS_NETWORK_AUDIO_DELAY:
			 *(int*)arg = PSystem.network_audio_delay;
			break;
		default:
			gxlogd("[PSYS] id = %d, Not Support Get !...\n",id);
			return ret;
		}

		return GX_PLAYER_OK;
	}

	return GxDevice_Get(devid, arg);
}

int GxPlayer_SystemFreezeFrameBufferAlloc(int id, unsigned int* addr, unsigned int* size)
{
	if(PSystem.freeze[id].fb_addr == 0 && PSystem.freeze[id].fb_size > 0) {
		PSystem.freeze[id].fb_addr = (unsigned int)av_memhole_malloc(PSystem.freeze[id].fb_size, GX_PINFLAG_FBVZ);
		if(PSystem.freeze[id].fb_addr == 0)
			return -1;
	}

	if (size) {
		if (*size > PSystem.freeze[id].fb_size)
			return -1;
		*size = PSystem.freeze[id].fb_size;
	}

	if (addr)
		*addr = PSystem.freeze[id].fb_addr;

	return 0;
}

int GxPlayer_SystemFreezeFrameBufferFree(void)
{
	int i;

	for (i = 0; i < sizeof(PSystem.freeze)/sizeof(PSystem.freeze[0]); i++) {
		if (PSystem.freeze[i].fb_addr) {
			av_memhole_free((void*)PSystem.freeze[i].fb_addr, GX_PINFLAG_FBVZ);
			PSystem.freeze[i].fb_addr = 0;
		}
	}

	return 0;
}


