#ifndef __GXPLAYER_CONFIG_H__
#define __GXPLAYER_CONFIG_H__

#include "gxplayer_module.h"
#include "gxcore.h"

__BEGIN_DECLS

/******************************* Video Start **********************************/

#define VIDEO_ESV_SD              ( 1024 * 1024 )
#define VIDEO_ESV_HD              ( 4096 * 1024 )
#define VIDEO_FB_SIZE             ((unsigned int)((1920 * 1088 * 2 + 16)* 12 + 8 * 1024)) //(yuv + check) * 12 + colbuf

#define VIDEO_DISPLAY_SCREEN      ( DISPLAY_SCREEN_16X9 )
#define VIDEO_ASPECT              ( ASPECT_NORMAL )
#define VIDEO_DRCMODE             ( GX_VDRC_TOSDR )
#define VIDEO_SCREEN_XRES         ( 1920 )
#define VIDEO_SCREEN_YRES         ( 1080 )

#define VIDEO_BRIGHTNESS          ( 50 )
#define VIDEO_SATURATION          ( 50 )
#define VIDEO_CONTRAST            ( 50 )

#define VIDEO_AUTO_ADAPT          ( 1 )
#define VIDEO_AUTO_ADAPT1         ( 1 )
#define VIDEO_AUTO_PAL            ( GX_VIDEO_OUTPUT_PAL )
#define VIDEO_AUTO_NTSC           ( GX_VIDEO_OUTPUT_NTSC_M )

#define VIDEO_SHARPNESS_EN        ( 0 )
#define VIDEO_HUE_EN              ( 0 )

#define VIDEO_CC_ENABLE     ( 0 )
#define VIDEO_CC_DISPLAY    ( 1 )
#define VIDEO_HDR_ENABLE    ( 0 )
#define VIDEO_USERDATA_LENGTH     ( 1024*2 )
#define VIDEO_MOSAIC_DROP_GATE    ( 0x7FFFFFFF )
#define VIDEO_MOSAIC_ERROR_GATE   ( 0 )
#define VIDEO_SYNC_FLAG           ( 1 )
#define VIDEO_SYNC_TIMEMS         ( 100 )
#define VIDEO_SYNC_HIGH_TIMEMS    ( 3000 )

#define VIDEO_SUPPORT_PPZOOM      ( 1 )
#define VIDEO_SUPPORT_DEINTERLACE ( 1 )
#define VIDEO_DEINTERLACE_WIDTH   ( 1280 )
#define VIDEO_DEINTERLACE_HEIGHT  ( 720 )

/******************************* Video End ************************************/


/******************************* Audio Start **********************************/

#define AUDIO_ESA_SD   ( 64  * 1024 )
#define AUDIO_ESA_HD   ( 128 * 1024 )

#define AUDIO_PCM_SD   ( 256 * 1024 )
#define AUDIO_PCM_HD   ( 512 * 1024 )


#define AUDIO_AC3_SD   ( AUDIO_ESA_SD )
#define AUDIO_DDP_SD   ( AUDIO_ESA_SD )
#define AUDIO_DTS_SD   ( AUDIO_ESA_SD )
#define AUDIO_AAC_SD   ( AUDIO_ESA_SD )
#define AUDIO_AC3_HD   ( AUDIO_ESA_HD )
#define AUDIO_DDP_HD   ( AUDIO_ESA_HD )
#define AUDIO_DTS_HD   ( AUDIO_ESA_HD )
#define AUDIO_AAC_HD   ( AUDIO_ESA_HD )

#define AUDIO_DMX_FULL_GATE     ( 1024 )
#define AUDIO_ANTI_ERROR_CODE   ( 1 )
#define AUDIO_SPDIF_SOURCE      ( SAFE_OUTPUT )
#define AUDIO_HDMI_SOURCE       ( SAFE_OUTPUT )
#define AUDIO_TRACK             ( AUDIO_TRACK_STEREO )
#define AUDIO_VOLUME_VALUE      ( 50 )
#define AUDIO_VOLTABLE_VALUE    ( 0 )
#define AUDIO_VOLUME_PRIVATE    ( 0 )
#define AUDIO_DOWNMIX           ( 1 )

#define AUDIO_BOOST_VOLUME_VALUE ( 32 )

#define AUDIO_AC3_DEFAULT_MODE  ( AUDIO_AC3_BYPASS_MODE )
#define AUDIO_AAC_DEFAULT_MODE  ( AUDIO_AAC_DECODE_MODE )
#define AUDIO_ENABLE_AC3_SD     ( 1 )
#define AUDIO_ENABLE_AC3_HD     ( 1 )
#define AUDIO_ENABLE_AAC_SD     ( 1 )
#define AUDIO_ENABLE_AAC_HD     ( 1 )
#define AUDIO_ENABLE_DTS_SD     ( 0 )
#define AUDIO_ENABLE_DTS_HD     ( 0 )
#define AUDIO_SYNC_FLAG         ( 1 )
#define AUDIO_SYNC_TIMEMS       ( 100 )
#define AUDIO_SYNC_HIGH_TIMEMS  ( 4000 )

#define AUDIO_SPEED_MUTE        ( 0 )
#define AUDIO_VOL_RIGHT_NOW     ( 0 )
#define AUDIO_VOL_STEP          ( 1 )

/******************************* Audio End ************************************/


/******************************* Subtitle Start *******************************/

#define SUBTITLE_ESS_SD   ( 64  * 1024 )
#define SUBTITLE_ESS_HD   ( 64  * 1024 )
#define SUBTITLE_ENABLE_SCTE    ( 0 )

/***************************** Subtitle End ***********************************/


/******************************* PVR   Start **********************************/

#define PVR_REC_CACHE_SD        ( 188 * 1024 * 5  )
#define PVR_REC_CACHE_HD        ( 188 * 1024 * 20 )

#define PVR_TIME_SHIFT_ENABLE   ( 0 )
#define PVR_TIME_SHIFT_DMXID    ( 0 )
#define PVR_TIME_SHIFT_FILE     ( "/mnt/usb0_1/shift.ts" )

#define PVR_RESERVE_SIZEMB      ( 20 )
#define PVR_VOLUME_SIZEMB       ( 200 )
#define PVR_VOLUME_MAX_NB       ( 0 )
#define PVR_VOLUME_FULLSTOP     ( 0 )

#define PVR_PLAY_DMXID          ( 1 )
#define PVR_PLAYBACK_HWTS_SYNCMODE       (PURE_APTS_RECOVER)
#define PVR_PLAYBACK_FORCE_HWTS    (1)
/******************************* PVR  End **************************************/


/******************************* Player Start **********************************/

#define PLAY_SUPPORT_NDS        ( 0 )
#define PLAY_SUPPORT_SVP        ( 0 )
#define PLAY_AUTO_PALY          ( 0 )
#define PLAY_ERROR_STOP         ( 0 )
#define PLAY_FREEZE_SWITCH      ( 1 )
#define PLAY_FREEZE_TIMEOUT     ( 3000000 )

#define PLAYER_HOLEMEM_SIZE     (0)

#define PLAYER_USERDATA_BUF_SIZE (2048)
/*                                      yuv                  +      colbuf       + userdata + mem_check */
#define PLAY_FREEZE_FB_SD       ( 768  * 576    * 1.5        +   768 * 576/4     + PLAYER_USERDATA_BUF_SIZE*2 +  16 )
#define PLAY_FREEZE_FB_HD       ( 1920 * 1088   * 1.5        +   1920*1088/4     + PLAYER_USERDATA_BUF_SIZE*2 +  16 )
#define PLAY_FREEZE_FB_HD_10BIT ( 1920 *(576*2) * 1.5 * 10/8 +  1920*(576*2)/16  + PLAYER_USERDATA_BUF_SIZE*2 +  16 )

#define PLAY_CACHE_SIZE_SD      ( 1024 * 1024 * 2 )
#define PLAY_CACHE_SIZE_HD      ( 1024 * 1024 * 10 )
#define INDEX_CACHE_SIZE_SD     ( 0x7FFFFFFF )
#define INDEX_CACHE_SIZE_HD     ( 0x7FFFFFFF )

#ifdef LINUX_OS
#define PLAY_CACHE_SIZE_NET          ( 1024 * 1024 * 4 )
#define PLAY_CACHE_SEEK_SIZE_NET     ( 512 * 1024 )
#else
#define PLAY_CACHE_SIZE_NET          ( 512 * 1024 )
#define PLAY_CACHE_SEEK_SIZE_NET     ( 64 * 1024 )
#endif
#define PLAY_NETWORK_TIMEOUT         ( 10*1000)   //ms
#ifdef LINUX_OS
#define PLAY_NETWORK_RECVBUF         (256*1024)   //256k
#define PLAY_MEDIA_MAX_STREAMS	     (40) //set media file, maximum number of supported streams.
#define PLAY_MPEGDASH_MAX_PERIODS	 (16) //maximum number of supported periods.
#else
#define PLAY_NETWORK_RECVBUF         ( 32*1024)   //32k
#define PLAY_MEDIA_MAX_STREAMS	     (32) //set media file, maximum number of supported streams.
#define PLAY_MPEGDASH_MAX_PERIODS	 (8) //maximum number of supported periods.
#endif

#define PLAY_STREAM_BUFFER_SIZE       (64*1024)
#define DEF_RECORD_TSW_MEMSIZE        (188*1024)

#define PLAY_PLAY_SOF_REPLAY         (1)

/*Set the default value for specifying bandwidth settings in the HLS/DASH protocol.*/
#define PLAY_NETWORK_RESOLUTION_WIDTH    (0)
#define PLAY_NETWORK_RESOLUTION_HEIGHT   (0)

/*Set the http:***.audio delay time.*/
#define PLAY_NETWORK_AUDIO_DELAY         (0)   //ms

/* V1: set hls url muliple list download,speed up dowload;V1 use not http keep-live mode;
 * notes:
 *        no. linux set 1.other set 0;
 *  V2: hls support http keep-live mode,ffmpeg defalut suuport mulitiple list, bat gxplayer limit NETSIZE size,
 *        configure multiple recvbuff cause memory is not enough.
 * Notes:
 *           Configuration:
 *                if (NETSIZE/2) > PLAY_NETWORK_RECVBUF*3   : set PLAY_NETWORK_HLS_MULTIPLE_LIST:1; 
 *                if (NETSIZE/2) <= PLAY_NETWORK_RECVBUF*3 : set PLAY_NETWORK_HLS_MULTIPLE_LIST:0; 
 *            Defalut configuration:  linux: Set 1;   Other: Set 0;
 */
#ifdef LINUX_OS
#define PLAY_NETWORK_HLS_MULTIPLE_LIST   (1)
#define PLAY_NETWORK_FFHLS_MAX_PLAYLIST  (6)
#else
#define PLAY_NETWORK_HLS_MULTIPLE_LIST   (0)
#define PLAY_NETWORK_FFHLS_MAX_PLAYLIST  (2)
#endif
#define PLAY_NETWORK_HTTP_PERSISTENT_CONNECTIONS  (1)


/*****************************************************************************************
 * When setting up a multi-bandwidth network flow, the upper layer can choose to set default resolution.
 *  Parameter Description:
 *          set value=1     :                   first max bandwidth
 *          set value=2     :                   second max bandwidth
 *          set other value(value > 0 and value!=1 and value !=2):         third max bandwidth
 *          set value=-1    :                   first min bandwidth
 *          set value=-2    :                   second min bandwidth
 *          set value=-3 or (value < 0 and value != -1 and value != -2)  :  third min bandwidth
 *****************************************************************************************/
#define PLAY_NETWORK_DEFAULT_BANDWIDTH_INDEX  (3)

/* false live mode.  is particularly noticable over high-latency network. */
/* e.g:dash live.hls live. defalut: not use.is 0.*/
#define PLAY_NETWORK_IS_FALSE_LIVE  (0)
/*http auto-adapability.use hls and mpegdash mulit bandwidth protocol.*/
#define PLAY_NETWORK_HTTP_AUTO_ADAPABILITY (0)
/*use hls and dash protocol.full detection for multiple sources.Used by some API customers.(For example:m k)*/
#define PLAY_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION (0)

/*default url size:4KB.*/
#define PLAY_DEFAULT_MAX_URL_SIZE (4096)
#define PLAY_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE (600)


#define PLAY_DELAY_CACHE_SIZE      ( 0x1000000 )
#define PLAY_DELAY_CACHE_HW_SIZE   ( 0x200000 )
#define PLAY_DELAY_CACHE_TO_MEMORY ( 0x1)
/******************************* Player End ************************************/


__END_DECLS

#endif
