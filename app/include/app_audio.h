#ifndef __APP_AUDIO_H__
#define __APP_AUDIO_H__

#include "app_config.h"
#include "service/gxplayer.h"
#include "module/si/si_pmt.h"

typedef struct
{
	uint32_t id;
	char *name;
	AudioCodecType audiotype;
}AudioList_t;

typedef struct
{
	AudioList_t *audioLangList;
	uint32_t audioLangCnt;
}AudioLang_t;

void app_track_set(PmtInfo* audio);
void app_audio_free(void);
AudioLang_t* app_get_audio_lang(void);
void app_audio_lang_free(AudioLang_t *audio_lang);
AudioCodecType app_audio_type(uint32_t elem_pid);
uint32_t app_audio_get_array_len(uint8_t *array, uint32_t strLen);
char *app_audio_get_str_duplication(uint8_t *array, uint32_t strLen);
char* app_audio_get_string_strcat(char *src,char *dst,char *separator);
#if AUDIO_DESCRIPTOR_SUPPORT
void app_ad_track_set(PmtInfo* audio);
void app_audio_description_free(void);
#endif

#endif
