
#ifndef __APP_MICROPHONE_VIEW_H__
#define __APP_MICROPHONE_VIEW_H__


#include "app_config.h"
#if USB_MICROPHONE_SUPPORT
//#include "alsa/sound/asound.h"

#define USB_MIKE_DEV_NAME    "USBMIKE"
#define USB_MIKE_AUTHOR      "Nationalchip"


typedef int /*__bitwise*/ snd_pcm_format_t1;
typedef int /*__bitwise*/ snd_pcm_access_t1;
#define ___force
#define SNDRV_PCM_FORMAT_S16_LE_ ((___force snd_pcm_format_t1) 2)
#define SNDRV_PCM_ACCESS_RW_INTERLEAVED_     ((___force snd_pcm_access_t1) 3) /* readi/writei */
#define FORMAT SNDRV_PCM_FORMAT_S16_LE
#define MIKE_CAPTURE_CHANNEL 1
#define MIKE_FORMAT SNDRV_PCM_FORMAT_S16_LE_
#define MIKE_RATE 44100
#define TYPE_PLAYBACK   16
#define MIKE_MAX_DEVICE_LEN 19
#define MIKE_TYPE_CAPTURE    24
#define MIKE_NAME_SIZE 20
#define ACCESS SNDRV_PCM_ACCESS_RW_INTERLEAVED_
#define CAPTURE_BUFFER_SIZE 64*1024
#define CAPTURE_PERIOD_SIZE 1024
#define PCM_CARD 0 // 声卡编号
#define PCM_DEVICE 0 // 设备编号
#define PCM_PLAYBACK 0 //1=播放,0=录音
#define PCM_PERIOD_SIZE 1024 // 每个周期的样本数
#define PCM_PERIOD_COUNT 4 // 周期数


// 定义一个新的结构体来封装 xferi 的内容
typedef struct {
    void *buf; // 数据缓冲区
    unsigned int frames; // 读取的帧数
} AudioCaptureResult;
typedef struct _VCPCMParamClass{
    unsigned short sample_rate;//ex. 16000, 44100, 48000 ...
    unsigned char channel_num;//ex. 2
    unsigned char bits;//ex. 16
    unsigned char is_signed;//ex. 1:signed, 0:unsigned
    unsigned char is_interlaced;//ex. 1:interlaced, 0:non-interlaced
    unsigned char is_LE;//ex. 1: little endian, 0: big endian
}VCPCMParamClass;
typedef struct _VCDongleClass{
    char *name;
    int id;// Vendor ID + Product ID，ex. 0x1d6ba4a7, vendor id=0x1d6b, product id=0xa4a7
    unsigned int uacp_support:1;// UAC Play
    unsigned int uacc_support:1;// UAC Capture
    unsigned int acmv_support:1;// ACM Audio Data parameters (voice_params)
    unsigned int reserved:29;
    VCPCMParamClass uacp_params;// playback params
    VCPCMParamClass uacc_params;// capture params
    VCPCMParamClass voice_params;// mic data
 }VCDongleClass;





extern int app_get_mute_state(void);
extern int app_mic_uac_device_in(char *name, int id);
extern int app_mic_uac_device_out(char *name, int id);
extern int app_get_mic_uac_device_hotplug_status(void);
extern status_t app_microphone_register_mike_dogle(void);
extern status_t app_microphone_unregister_mike_dogle(void);
extern int app_microphone_audio_enable(void);
extern int app_microphone_pause(void);
extern int app_microphone_play(void);
extern int app_microphone_set_mute(int mute);
extern int app_microphone_set_channel(int channel);
extern int app_microphone_set_volume(int volume);
extern int app_microphone_set_mute_flag(int mute);
extern int app_microphone_get_mute_flag(void);


#endif
#endif
