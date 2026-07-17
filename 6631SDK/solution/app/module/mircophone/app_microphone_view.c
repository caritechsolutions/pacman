#include "app_module.h"
#include "app_windows.h"
#include "full_screen.h"
#if USB_MICROPHONE_SUPPORT
#include <fcntl.h>
#include "devapi/uac_api.h"
#include "stdlib.h"
#include "bus/module/player/gxaudio_dongle.h"
#include "bus/service/gxusbbtplug.h"
#include "app_microphone_view.h"


static unsigned int thiz_uac_capt_fd = 0;
static int thiz_uac_hotplug_status = -1;
static int thiz_uac_device_num  = 0;
static gx_uac_handle_t *thiz_pcm = NULL;
static int thiz_mute_flag = 0;

VCDongleClass thiz_microphone_ops = {
    .name = "gx_microphone",
    .id = 0xffe0001,
    .acmv_support = 1,
    .voice_params ={
        .sample_rate    = MIKE_RATE,
        .channel_num    = MIKE_CAPTURE_CHANNEL,// mic
        .bits           = 16,
        .is_signed      = 1,// signed
        .is_interlaced  = 0,// none interlaced
        .is_LE          = 1,// little endian
    },
};

typedef struct _VCDeviceInforClass{
    char uacp_name[MIKE_NAME_SIZE];//ex. "/dev/pcmC0D0p"
    char uacc_name[MIKE_NAME_SIZE];//ex. "/dev/pcmC0D1c"
    char acm_name[MIKE_NAME_SIZE];//ex. "/dev/ttyUSB0"
    VCDongleClass *dongle;
}VCDeviceInforClass;

static VCDeviceInforClass thiz_dongles[] = {
    {
        .uacp_name = {'/','d','e','v','/','p','c','m','C','0','D','0'},//"/dev/pcmC0D0",
        .uacc_name = {'/','d','e','v','/','p','c','m','C','0','D','0','c'},//"/dev/pcmC0D0c",//dev/pcmC0D1c",
        .acm_name = {[0 ... 19] = 0},
        .dongle = &thiz_microphone_ops,
    },
};
static VCDongleClass *thiz_dongle_ops = &thiz_microphone_ops;


//---------V_UAC
static int _is_uacp(char *device_name)
{
    int bytes = 0;
    if((NULL == device_name)
            || (13 > (bytes = strlen(device_name)))
            || (device_name[bytes-1] != 'p'))
        return 0;

    if(strncmp(device_name, "/dev/pcmC", 9))
        return 0;

    return 1;
}
static int _is_uacc(char *device_name)
{
    int bytes = 0;
    if((NULL == device_name)
            || (13 > (bytes = strlen(device_name)))
            || (device_name[bytes-1] != 'c'))
        return 0;

    if(strncmp(device_name, "/dev/pcmC", 9))
        return 0;

    return 1;
}

static int _add_uacp(char *device_name, VCDeviceInforClass *dongle)
{
    char *p = device_name;
    unsigned int bytes = 0;
    int ret = 0;

    if(strlen(dongle->uacp_name))// exist
        return -1;
    p = strchr(p,'D');
    bytes = p - device_name;
    if(strlen(dongle->uacc_name))
    {
        if(memcmp(device_name, dongle->uacc_name, bytes))
            return -1;
        else if(strlen(dongle->acm_name))
            ret = 1;
        else
            ret = -1;
    }
    bytes = strlen(device_name);
    memset(dongle->uacp_name, 0, sizeof(dongle->uacp_name));
    memcpy(dongle->uacp_name, device_name, bytes);

    return ret;
}

static int _add_uacc(char *device_name, VCDeviceInforClass *dongle)
{
    char *p = device_name;
    unsigned int bytes = 0;
    int ret = 0;

    if(strlen(dongle->uacc_name))// exist
        return -1;
    p = strchr(p,'D');
    bytes = p - device_name;
    if(strlen(dongle->uacp_name))
    {
        if(memcmp(device_name, dongle->uacp_name, bytes))
            return -1;
        else if(strlen(dongle->acm_name))
            ret = 1;
        else
            ret = -1;
    }
    bytes = strlen(device_name);
    memset(dongle->uacc_name, 0, sizeof(dongle->uacc_name));
    memcpy(dongle->uacc_name, device_name, bytes);

    return ret;
}

static int extract_integer(char *str)
{
    // 找到第一个 'D' 字符的位置
    char *d_pos = strchr(str, 'D');
    if (d_pos != NULL) {
        // 从 'D' 后面开始查找数字
        int value = atoi(d_pos + 1);
        return value;
    }
    return -1; // 如果没有找到 'D'，返回 -1 表示错误
}
static VCDongleClass *_voice_control_device_in(char *device_name, int device_id)
{
    int i = 0;
    int is_uacp = 0;
    int is_uacc = 0;
    unsigned int count = sizeof(thiz_dongles)/sizeof(VCDeviceInforClass);

    if((NULL == device_name) || (MIKE_MAX_DEVICE_LEN < strlen(device_name)))
        return NULL;

    is_uacp = _is_uacp(device_name);
    if(0 == is_uacp)
    {
        is_uacc = _is_uacc(device_name);
        if(0 == is_uacc)
        {
            return NULL;
        }
    }

    for(i = 0; i < count; i++)
    {
        if((thiz_dongles[i].dongle) && (device_id == thiz_dongles[i].dongle->id))
        {// find the match dongle
            if(is_uacp)
            {// uacp
                if(1 == _add_uacp(device_name, &thiz_dongles[i]))
                    return thiz_dongles[i].dongle;
            }
            else if(is_uacc)
            {// uacc
                if(1 == _add_uacc(device_name, &thiz_dongles[i]))
                    return thiz_dongles[i].dongle;
            }
            break;
        }
    }
    return NULL;
}

int app_mic_uac_device_in(char *name, int id)
{
    VCDongleClass *ops = NULL;
    app_log_info("line:%d,id=%d,name=%s\n",__LINE__,id,name);
    ops = _voice_control_device_in(name, id);
    thiz_uac_device_num = extract_integer("/dev/pcmC0D1c");
    thiz_uac_device_num = extract_integer(name);
    thiz_uac_hotplug_status = id;
    if((ops) && (NULL == thiz_dongle_ops))
    {
        thiz_dongle_ops = ops;
        thiz_dongle_ops->name = strdup(name);
        return 0;
    }
    return -1;
}

int app_mic_uac_device_out(char *name, int id)
{
    app_microphone_pause();
    thiz_uac_hotplug_status = -1;
    thiz_uac_capt_fd = 0;
    return -1;
}

int app_get_mic_uac_device_hotplug_status(void)
{
    return thiz_uac_hotplug_status;
}

static VCDeviceInforClass *_get_device_block_by_id(int device_id)
{
    int i = 0;
    unsigned int count = sizeof(thiz_dongles)/sizeof(VCDeviceInforClass);
    for( i = 0; i < count; i++ )
    {
        if((thiz_dongles[i].dongle) && (device_id == thiz_dongles[i].dongle->id))
            return &thiz_dongles[i];
    }
    return NULL;
}

static int vc_get_uacc_params(char **device_name, VCPCMParamClass **params)
{
    VCDeviceInforClass *device_infor = NULL;
    if((NULL == device_name) || (NULL == params) || (NULL == thiz_dongle_ops))
    {
        return -1;
    }
    device_infor = _get_device_block_by_id(thiz_dongle_ops->id);
    if((NULL == device_infor) || (0 == strlen(device_infor->uacc_name)))
    {
        return -1;
    }
    *device_name = device_infor->uacc_name;
    *params = &(thiz_dongle_ops->uacc_params);
    return 0;
}

static int _uac_audio_capture_init(void)
{
    char *uacc_dev_name = NULL;
    VCPCMParamClass *uacc_params = NULL;
    if((thiz_uac_capt_fd > 0)
            || (0 != vc_get_uacc_params(&uacc_dev_name, &uacc_params)))
        return -1;
	gx_uac_config_t config = {
		.channels = MIKE_CAPTURE_CHANNEL,
		.rate = MIKE_RATE,
		.format = UAC_FORMAT_S16_LE,
		.period_size = PCM_PERIOD_SIZE,
		.period_count = PCM_PERIOD_COUNT
	};
	// 打开PCM设备
	thiz_pcm = GxCore_UAC_Open(PCM_CARD, thiz_uac_device_num, PCM_PLAYBACK, &config);
	if (!thiz_pcm) {
		app_log_error("Failed to open PCM device\n");
		return -1;
	}
    usleep(100000);  // 等待100ms让设备稳定
    thiz_uac_capt_fd = (unsigned int)thiz_pcm;

    return 0;
}

static int _uac_capture_close(void)
{
    if(thiz_pcm) {
        GxCore_UAC_Stop(thiz_pcm);
        GxCore_UAC_Close(thiz_pcm);
    }
    thiz_pcm = NULL;
    thiz_uac_capt_fd = 0;
    return 0;
}

int _uac_audio_capture(unsigned char *buf, unsigned int size)
{
    int ret = 0;
    int avail = 0;
    avail = GxCore_UAC_GetBufAvail(thiz_pcm);
    if (avail < size) {
        ret = GxCore_UAC_Read(thiz_pcm, buf, GxCore_UAC_BytesToFrames(thiz_pcm, avail));
        return GxCore_UAC_FramesToBytes(thiz_pcm, ret);
    } else {
        ret = GxCore_UAC_Read(thiz_pcm, buf, GxCore_UAC_BytesToFrames(thiz_pcm, size));
        //app_log_info(">>>>>>>>>>>>>>>>>>>====ret=%d\n",ret);
        return GxCore_UAC_FramesToBytes(thiz_pcm, ret);
    }
    return ret;
}

static unsigned int app_microphone_get_cap(GxAudioDongleCap  *cap)
{
    cap->format = GXAUDIO_DONGLE_PCM;
    cap->keep_pulse = 1;
    cap->pcm.sample_freq = MIKE_RATE;
    cap->pcm.channel_num = MIKE_CAPTURE_CHANNEL;
    cap->pcm.bits        = 16;
    cap->pcm.interlace   = 1;
    cap->pcm.endian      = 0;

    return 0;
}

static unsigned int app_microphone_get_bufinfo(GxAudioDongleBufInfo *data)
{
    unsigned int len;
    unsigned char *buf;
    unsigned int read_len = GxCore_UAC_GetBufSize(thiz_pcm);

    if (read_len == 0) {
        app_log_error("Error: Buffer size is zero.\n");
        return 1; // 返回错误码
    }

    buf = (unsigned char *)GxCore_Malloc(read_len);
    if (!buf) {
        app_log_error("Error: Memory allocation failed.\n");
        return 2; // 返回不同的错误码
    }

    len = read_len;
    while (len == read_len) {
        len = _uac_audio_capture(buf, read_len);
        if (len < 0) {
            app_log_error("Error: Audio capture failed.\n");
            GxCore_Free(buf);
            return 3; // 返回不同的错误码
        }
        app_log_info("Captured length: %d\n", len);
    }

    // 假设这里需要更新 data 的信息
    data->datalen = 0; // 更新为实际数据长度
    data->freelen = 0; // 更新为实际空闲长度

    GxCore_Free(buf);
    return 0;
}

static int app_microphone_pull_data(GxAudioDongleData *data)
{
#define READ_LEN0 (4)
    unsigned char buf[READ_LEN0];
    unsigned int len = READ_LEN0;
    _uac_audio_capture(buf, len);
    //app_log_info("<<<<<<<<<< size %d,buf_prt=%s\n", data->buf_size,data->buf_ptr);
    return _uac_audio_capture(data->buf_ptr, data->buf_size);
}

static GxAudioDongleOps thiz_mike_ops = {
    .name = USB_MIKE_DEV_NAME,
    .private_name = GX_AUDIO_DONGLE_OUT_PCM_FRAME_NAME,
    .author = USB_MIKE_AUTHOR,
    .get_cap = app_microphone_get_cap,
    .get_bufinfo = app_microphone_get_bufinfo,
    .push = NULL,
    .pull = app_microphone_pull_data,
};

status_t app_microphone_register_mike_dogle(void)
{
    GxAudioRegisterDongle(&thiz_mike_ops);
    return GXCORE_SUCCESS;
}

status_t app_microphone_unregister_mike_dogle(void)
{
    GxAudioUnRegisterDongle(&thiz_mike_ops);
    return GXCORE_SUCCESS;
}

int app_microphone_audio_enable(void)
{
    int ret = -2;
    int32_t init_value = 0;
    GxAudioDongleParam param;

    _uac_audio_capture_init();
    if(thiz_uac_capt_fd > 0){
        GxBus_ConfigGetInt(USB_AUDIO_VOLUME_KEY, &init_value, AUDIO_VOLUME);
        param.mute = app_get_mute_state();
        param.volume = init_value;
        param.channel = MIKE_CAPTURE_CHANNEL;
        ret = GxAudioDongleEnable(thiz_mike_ops.name,&param);
    }
    return ret;
}

int app_microphone_pause(void)
{
    int ret = 0;
    GxAudioDongleDisable(thiz_mike_ops.name);
    ret = _uac_capture_close();
    return ret;
}

int app_microphone_play(void)
{
    int ret = 0;

    app_microphone_audio_enable();
    return ret;
}


int app_microphone_set_mute(int mute)
{
    int ret = 0;
    char *img_name = NULL;
    FullArb *arb = &g_AppFullArb;

    app_microphone_set_mute_flag(mute ? 0 : 1);
    int mic_mute = app_microphone_get_mute_flag();
    ret = GxAudioDongleSetMute(thiz_mike_ops.name, mic_mute);

    if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW)) {
        img_name = "music_view_imag_mute";
    } else if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW)) {
        img_name = "movie_view_imag_mute";
    } else if ((GXCORE_SUCCESS != GUI_CheckDialog(WND_MAIN_MENU))
                && (GXCORE_SUCCESS != GUI_CheckDialog(WND_EPG))
                && (GXCORE_SUCCESS != GUI_CheckDialog(WND_CHANNEL_EDIT))
                && (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MEDIA_CENTRE))) {
        img_name = "img_full_mute";
    } else {
        return 0;
    }

    if (img_name) {
        // 如果麦克风或节目处于静音状态，显示静音标识
        if (mic_mute || arb->state.mute) {
            GUI_SetProperty(img_name, "state", "show");
        } else {
            GUI_SetProperty(img_name, "state", "hide");
        }
        GUI_SetProperty(img_name, "draw_now", NULL);
    }

    return ret;
}

int app_microphone_set_volume(int volume)
{
    int ret = 0;

    ret = GxAudioDongleSetVolume(thiz_mike_ops.name,volume);
    return ret;
}

int app_microphone_set_channel(int channel)
{
    int ret = 0;

    ret = GxAudioDongleSetChannel(thiz_mike_ops.name,channel);
    return ret;

}

int app_microphone_set_mute_flag(int mute)
{
    thiz_mute_flag = mute;
    return 0;
}

int app_microphone_get_mute_flag(void)
{
    return thiz_mute_flag;
}
#endif
