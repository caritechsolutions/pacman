#include "app_fm_record.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_fm.h"

#if DVB2IP_SERVER_SUPPORT && FM_SUPPORT

//int32_t wav_file_fd = -1;
static int32_t read_data_times = 0;

static int32_t _fm_buffer_config(void)
{
    int fe_handle = -1;
    int tuner_id = 0;
    struct fe_buffer_parameters params= {0};

    tuner_id = app_get_fm_tuner_id();
    fe_handle = GxFrontend_IdToHandle(tuner_id);
    if (fe_handle < 0)
        return -1;

    params.buffer_threshold = 16*1024;
    params.sw_buffer_size = 1024*1024;
    params.hw_buffer_size = 256*1024;
    return ioctl(fe_handle, FE_CONFIG_BUFFER, &params);
}

int32_t app_fm_record_init(int32_t fifo_size, int32_t max_prog)
{
    int handle = -1;
    int tuner = 0;
    AppFrontend_Monitor monitor;

    tuner = app_tuner_cur_tuner_get();
    monitor = FRONTEND_MONITOR_OFF;
    app_ioctl(tuner, FRONTEND_MONITOR_SET, &monitor);
    GxFrontend_SetUnlock(tuner);
    GxFrontend_CleanRecordPara(tuner);

    if(padmux_check(34, 4) != 0)
        padmux_set(34, 4);

    handle = app_fm_get_handle();
    if(handle < 0)
        return -1;
    GxFeInitFM(handle, fifo_size, max_prog);

    if(_fm_buffer_config() < 0)
        return -1;


    return 0;
}

static int32_t _fm_rec_lock_tp(int32_t freq_KHz)
{
    GxFeTpParameters tp_params;
    int handle = -1;

    handle = app_fm_get_handle();
    if(handle < 0)
        return -1;

    memset(&tp_params, 0, sizeof(GxFeTpParameters));
    tp_params.frequency_KHz = freq_KHz;
    tp_params.type          = FE_FM;
    return GxFeLockTp(handle, tp_params);
}

handle_t app_fm_record_start(FmRecConfig *config, int32_t need_lock_tp)
{
    uint32_t freq_KHz;
    char fm_url[100]={0};
    int32_t tuner = 0;
    void* ret;
    if(NULL == config)
        return 0;
    freq_KHz = app_fm_get_freq(config->index-1);
    tuner = app_get_fm_tuner_id();

    if(freq_KHz<=0)
        return 0;
    sprintf(fm_url, "fm://fre:%d&tuner:%d", freq_KHz, tuner);
    ret = GxPlayer_StreamOpen(fm_url, GX_PLAYER_STREAM_RONLY);
    if(need_lock_tp == 1){
        if(config->index > 0){
            if(_fm_rec_lock_tp(freq_KHz) < 0)
                return 0;
        }else
                return 0;

    }
    read_data_times = 0;

    return ret == NULL?0:(handle_t)ret;
}

uint8_t* pcm_to_wav(uint8_t *pcm, int32_t pcmlen, int32_t *wavlen)
{
    //44字节wav头
    uint8_t *wav = GxCore_Calloc(pcmlen + 44, sizeof(uint8_t));
    //wav文件多了44个字节
    *wavlen = pcmlen + 44;
    //添加wav文件头
    memcpy(wav, "RIFF", 4);
    *(int *)(wav + 4) = pcmlen + 36;
    memcpy((wav + 8), "WAVEfmt ", 8);
    *(int *)(wav + 16) = 16;
    *(short *)(wav + 20) = 1;
    *(short *)(wav + 22) = 1;
    *(int *)(wav + 24) = 48000;
    *(int *)(wav + 28) = 96000;
    *(short *)(wav + 32) = 16 / 8;
    *(short *)(wav + 34) = 16;
    strcpy((char*)(wav + 36), "data");
    *(int *)(wav + 40) = pcmlen;

    //拷贝pcm数据到wav中
    memcpy(wav + 44, pcm, pcmlen);
    return wav;
}

int32_t app_fm_record_read(handle_t handle, uint8_t *buffer, int32_t size)
{
    uint8_t *fifo_buffer = NULL;
    int32_t wav_len = 0;
    int32_t ret = 0;
    GxFeDataParams data = {0};

    if(NULL == buffer || 0 == size)
        return ret;

    data.buffer   = buffer;
    data.read_len = size;
    data.real_len = 0;

    //GxFeReadData(handle, &data);
    //ret = data.real_len;
    ret = GxPlayer_StreamRead((void*)handle, data.buffer, data.read_len);
    //printf("GxFeReadData read_len:%d, real_len:%d\n", size, ret);
    if(read_data_times == 0)
    {
        read_data_times++;
        fifo_buffer = pcm_to_wav(buffer, ret, &wav_len);
        ret = (wav_len <= size) ? wav_len : size;
        memcpy(buffer, fifo_buffer, ret);
    }

    if(fifo_buffer)
    {
        GxCore_Free(fifo_buffer);
        fifo_buffer = NULL;
    }
    return ret;
}

void app_fm_record_stop(handle_t handle)
{
    GxPlayer_StreamClose((void*)handle);
    //GxFeCloseFMOneChannel(handle);
    return;
}

void app_fm_record_destroy(void)
{
    int handle = -1;

    if(padmux_check(34, 1) != 0)
        padmux_set(34, 1);


    handle = app_fm_get_handle();
    if(handle < 0)
        return;
    return;
}

#endif
