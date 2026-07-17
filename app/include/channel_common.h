#ifndef __CHANNEL_COMMON_H__
#define __CHANNEL_COMMON_H__

#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "module/channel_edit.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "app_config.h"
#include "app_book.h"
#include "app_keyboard_language.h"
#include "app_utility.h"

#define FAV_MASK    (0x80000000)
#define ALL_SAT     0x5A5A5A5A
#define HD_MASK     (0xA5A5A5A5)

typedef enum {
	APP_PM_PROG_MPEG = 0, // mepg2编码
	APP_PM_PROG_AVS,      // AVS编码
	APP_PM_PROG_H264,     // h.264编码
	APP_PM_PROG_H265,     // h.265编码
	APP_PM_PROG_MPEG4,    // mpeg4编码
}AppProgVideoType;

typedef enum {
	APP_PM_AUDIO_MPEG1 = 0,    // mpeg-1
	APP_PM_AUDIO_MPEG2,        // mpeg-2
	APP_PM_AUDIO_AAC_LATM,     // aac??频
	APP_PM_AUDIO_AC3,          // ac3??频
	APP_PM_AUDIO_AAC_ADTS,     // aac??频
	APP_PM_AUDIO_EAC3,         // eac3
	APP_PM_AUDIO_LPCM,
	APP_PM_AUDIO_DTS,
	APP_PM_AUDIO_DOLBY_TRUEHD,
	APP_PM_AUDIO_AC3_PLUS,
	APP_PM_AUDIO_DTS_HD,
	APP_PM_AUDIO_DTS_MA,
	APP_PM_AUDIO_AC3_PLUS_SEC,
	APP_PM_AUDIO_DTS_HD_SEC,
	APP_PM_AUDIO_DRA,
}AppProgAudioType;


GxBusPmDataProgVideoType channel_get_bus_video_type(AppProgVideoType video_type);
AppProgVideoType channel_get_app_video_type(GxBusPmDataProgVideoType video_type);

GxBusPmDataProgAudioType channel_get_bus_audio_type(AppProgAudioType audio_type);
AppProgAudioType channel_get_app_audio_type(GxBusPmDataProgAudioType audio_type);

void channel_get_video_type_str(char* video_str,AppProgVideoType video_type,int str_len);
void channel_get_audio_type_str(char* audio_str,AppProgAudioType audio_type,int str_len);

int channel_id_total_and_buffer_dump(AppIdOps *id_ops, uint32_t *id_total, uint32_t **id_buffer);
uint32_t channel_get_cur_group_num(AppIdOps *cl_id);
int channel_fav_flag_to_id_add(AppIdOps *id_ops ,uint32_t flag);
int channel_get_valid_group_id(AppIdOps *cl_id, AppIdOps *sat_id, AppIdOps *fav_id, AppIdOps *hd_id, GxBusPmViewInfoSkipView view_mode);
int channel_cmb_group_content_dump(AppIdOps *cl_id, char **buffer_all);
void channel_sat_tp_info_str_get(GxBusPmDataSat *sat_data, GxBusPmDataTP *tp_data, char *buffer, int len);
void channel_prog_tp_info_str_get(GxBusPmDataProg *prog_data, char* buffer, int len);
int channel_set_group_fav_hd_type(GxMsgProperty_NodeByPosGet prog_node,int hd_type);
char *app_get_sat_name(GxBusPmDataSat *sat);
int channel_get_playing_pos_in_group(uint32_t *ppos);
#if WEBAPI_SUPPORT
int webapi_channel_get_group_id(AppIdOps *cl_id, GxBusPmDataProgType  stream_type);
#endif
#endif

