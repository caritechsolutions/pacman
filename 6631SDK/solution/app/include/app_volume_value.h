#ifndef __APP_VOLUME_VALUE_H__
#define __APP_VOLUME_VALUE_H__

#define MAX_DISPLAY_VOL (25)

// 0-40 映射成0-100
#define AUDIO_MAP_40(audio) (((audio)*5)>>1)

//3: 0-25 映射成0-75 4: 0-25 映射成0-100
#define VOLUME_NUM (4)
#define AUDIO_MAP_25(audio) ((audio) * VOLUME_NUM)

// 0-50 to 0-75
#define AUDIO_MAP_50(audio) ((audio)/2 + (audio))

status_t app_volume_scope_status(void);
status_t app_system_vol_save(GxMsgProperty_PlayerAudioVolume val);
status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val);
status_t app_system_vol_set_val(GxMsgProperty_PlayerAudioVolume val);
uint32_t app_system_vol_get(void);
void app_volume_value_init(void);
void app_volume_value_set(int volume);

#endif

