#ifndef __SYS_SETTING_MANAGE__
#define __SYS_SETTING_MANAGE__


typedef enum
{
    TvAuto,
    TvPal,
    TvNtsc,
    TvTypeCnt
}AvTvTypeSettings;

void system_setting_set_screen_ratio(int32_t screenRatio);

void system_setting_set_screen_type(int32_t screenType);

void system_setting_set_screen_color(uint32_t chroma,uint32_t brightness,uint32_t contrast,
         uint32_t sharpness, uint32_t hue, bool realtime);

void system_setting_set_tv_type_no_auto(uint32_t tvType,uint32_t output);

void system_setting_set_full_gate(int init_value);


#endif
