#include "gxplayer_internal.h"
#include "gx_audio_control.h"

status_t GxPlayer_DebugNetworkStream(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_NETWORK_STREAM, &enable);
}

status_t GxPlayer_DebugNetworkRTPPacket(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_RTP_PACKET, &enable);
}

status_t GxPlayer_DebugStreamSpeed(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_STREAM_SPEED, &enable);
}

status_t GxPlayer_DebugStreamPVR(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_STREAM_PVR, &enable);
}

status_t GxPlayer_DebugPacketSpeed(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_PACKET_SPEED, &enable);
}

status_t GxPlayer_DebugDemuxFillData(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_DEMUX_FILL_DATA, &enable);
}

status_t GxPlayer_DebugDemuxApts(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_DEMUX_APTS, &enable);
}

status_t GxPlayer_DebugDemuxVpts(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_DEMUX_VPTS, &enable);
}

status_t GxPlayer_DebugPlayerMedia(void)
{
	av_debug_media_printf();
	return GX_PLAYER_OK;
}

status_t GxPlayer_DebugPlayerCostTime(void)
{
	av_debug_costtime_printf();
	return GX_PLAYER_OK;
}

status_t GxPlayer_DebugAdecConfig(PlayerDebugAVLevel level)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_ADEC_CONFIG, &level);
}

status_t GxPlayer_DebugAdecContent(PlayerDebugAdecContent content)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_ADEC_CONTENT, &content);
}

status_t GxPlayer_DebugAoutConfig(PlayerDebugAVLevel level)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_AOUT_CONFIG, &level);
}

status_t GxPlayer_DebugAoutContent(PlayerDebugAoutContent content)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_AOUT_CONTENT, &content);
}

status_t GxPlayer_DebugSubtitleSync(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_SUBTITLE_SYNC, &enable);
}

status_t GxPlayer_DebugAVInfo(int enable)
{
	return GxPlayer_SystemSet(PSYS_DEBUG_AV_INFO, &enable);
}

status_t GxPlayer_DumpAdecPCM(int enable, char *url)
{
	return gx_audio_record_adec_pcm(enable, url);
}

status_t GxPlayer_DumpAdecESA(int enable, char *url)
{
	return gx_audio_record_adec_esa(enable, url);
}

status_t GxPlayer_DumpAoutPCM(int enable, char *url)
{
	return gx_audio_record_aout_pcm(enable, url);
}

void GxPlayer_HeapInfo(void)
{
	extern void gxplayer_heap_info(void);
	gxplayer_heap_info();
}

void GxPlayer_MemholeInfo(GxMemholeInfo *info)
{
	if (info) {
		info->now_used = av_memhole_now_used();
		info->max_used = av_memhole_max_used();
	}
}
