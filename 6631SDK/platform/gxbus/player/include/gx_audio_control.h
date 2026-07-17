#ifndef __GX_DEBUG_DUMP_H__
#define __GX_DEBUG_DUMP_H__

status_t gx_audio_record_adec_pcm(int enable, char *url);

status_t gx_audio_record_adec_esa(int enable, char *url);

status_t gx_audio_record_aout_pcm(int enable, char *url);

status_t gx_audio_smart_volume(int enable, AudioSmartConfig *config);

status_t gx_audio_smart_volume_set_loudness(float  loudness);

status_t gx_audio_smart_volume_get_loudness(float *loudness);

#endif
