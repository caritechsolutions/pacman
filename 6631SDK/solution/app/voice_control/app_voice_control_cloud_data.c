#include "gxcore.h"

typedef enum _VCCloudStatusEnum{
    VC_CLOUD_WORKING = 1,
    VC_CLOUD_IDLE,
    VC_CLOUD_NONE,
}VCCloudStatusEnum;

#define DATA_GET_MAX            6
typedef struct _VCCloudControlClass{
    char id;// from 1 to DATA_GET_MAX
    char active_flag;
}VCCloudControlClass;

#define DATA_FRINT              printf
#define DATA_CACHE_BASIC_SIZE   512*1024
#define DATA_CACHE_MAX_SZIE     512*1024*4
static VCCloudStatusEnum thiz_vc_data_status = VC_CLOUD_NONE;
static int thiz_data_mutex = -1;
static char *thiz_data_cache = NULL;
static int thiz_data_size = 0;
static int thiz_data_cache_size = DATA_CACHE_BASIC_SIZE;// bytes
static VCCloudControlClass thiz_handle_control[DATA_GET_MAX];
static int thiz_init_flag = 0;

static int _request_id(void)
{
    int i = 0;
    for(i = 0; i < DATA_GET_MAX; i++)
    {
        if(thiz_handle_control[i].id == 0)
        {
            return (i+1);
        }
    }
    return 0;
}

static int _add_to_map(int handle)
{
    int i = 0;
    for(i = 0; i < DATA_GET_MAX; i++)
    {
        if((thiz_handle_control[i].id != 0)
                && (thiz_handle_control[i].active_flag))
        {
            thiz_handle_control[i].active_flag = 0;
        }
    }

    thiz_handle_control[handle-1].id = handle;
    thiz_handle_control[handle-1].active_flag = 1;
    return 0;
}

static int _delete_from_map(int handle)
{
    int i = 0;

    thiz_handle_control[handle-1].id = 0;
    thiz_handle_control[handle-1].active_flag = 0;
    for(i = (DATA_GET_MAX - 1); i >= 0; i--)
    {
        if(thiz_handle_control[i].id != 0)
        {
            thiz_handle_control[i].active_flag = 1;
            break;
        }
    }
    return 0;
}

#define _set_active    _add_to_map

int app_vc_cloud_init(void)
{
    GxCore_MutexCreate(&thiz_data_mutex);
    if(0 >= &thiz_data_mutex)
    {
        return -1;
    }
    memset(thiz_handle_control, 0, sizeof(VCCloudControlClass)*DATA_GET_MAX);
    thiz_init_flag = 1;
    return 0;
}

int app_vc_cloud_destroy(void)
{
    if(0 == thiz_init_flag)
        return -1;
    GxCore_MutexLock(thiz_data_mutex);
    thiz_vc_data_status = VC_CLOUD_NONE;
    thiz_data_cache_size = DATA_CACHE_BASIC_SIZE;
    thiz_data_size = 0;
    if(thiz_data_cache)
    {
        GxCore_Free(thiz_data_cache);
        thiz_data_cache = NULL;
    }
    GxCore_MutexUnlock(thiz_data_mutex);
    GxCore_MutexDelete(thiz_data_mutex);
    thiz_data_mutex = -1;
    return 0;
}


int app_vc_cloud_open(int *handle)
{
    int id = 0;
    if((NULL == handle) || (0 == thiz_init_flag))
    {
        DATA_FRINT("\nParameters error...\n");
        return -1;
    }

    GxCore_MutexLock(thiz_data_mutex);
    id = _request_id();
    if(id > 0)
    {
        *handle = id;
        _add_to_map(id);
        GxCore_MutexUnlock(thiz_data_mutex);
        return 0;
    }
    GxCore_MutexUnlock(thiz_data_mutex);
    DATA_FRINT("\nControl block full...\n");
    return -1;
}

int app_vc_cloud_close(int handle)
{
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    if((0 > handle) || (DATA_GET_MAX < handle) || (handle != thiz_handle_control[handle -1].id))
    {
        DATA_FRINT("\nParameters error...\n");
        GxCore_MutexUnlock(thiz_data_mutex);
        return -1;
    }
    _delete_from_map(handle);
    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
}

int app_vc_cloud_active(int handle)
{
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    if((0 > handle) || (DATA_GET_MAX < handle) || (handle != thiz_handle_control[handle -1].id))
    {
        GxCore_MutexUnlock(thiz_data_mutex);
        DATA_FRINT("\nParameters error...\n");
        return -1;
    }
    _set_active(handle);
    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
}

// for APPs
static void _fill_wave_header(unsigned char *buf, unsigned int bitwidth, unsigned int channels, unsigned int sample_rate, unsigned int voice_data_size)// 默认小端
{// 44 Bytes
    unsigned char *ptr = buf;
    unsigned int  sample_byte = (bitwidth / 8) * channels;
    unsigned int  sample_size = sample_rate * sample_byte;

    // ChunkID
    ptr[0] = 'R'; ptr[1] = 'I'; ptr[2] = 'F'; ptr[3] = 'F';
    ptr += 4;

    // ChunkSize
    ptr[0] = (((voice_data_size + 44 - 8) >>  0) & 0xff);
    ptr[1] = (((voice_data_size + 44 - 8) >>  8) & 0xff);
    ptr[2] = (((voice_data_size + 44 - 8) >>  16) & 0xff);
    ptr[3] = (((voice_data_size + 44 - 8) >>  24) & 0xff);
    ptr += 4;

    // Format
    ptr[0] = 'W'; ptr[1] = 'A'; ptr[2] = 'V'; ptr[3] = 'E';
    ptr += 4;

    // Subchunk1ID
    ptr[0] = 'f'; ptr[1] = 'm'; ptr[2] = 't'; ptr[3] = ' ';
    ptr += 4;

    // Subchunk1Size: 16bytes
    ptr[0] = 0x10; ptr[1] = 0x00; ptr[2] = 0x00; ptr[3] = 0x00;
    ptr += 4;

    // AudioFormat: 1
    ptr[0] = 0x01;
    ptr[1] = 0x00;

    // NumChannels
    ptr[2] = ((channels >> 0) & 0xff);
    ptr[3] = ((channels >> 8) & 0xff);
    ptr += 4;

    // SampleRate
    ptr[0] = ((sample_rate >>  0) & 0xff);
    ptr[1] = ((sample_rate >>  8) & 0xff);
    ptr[2] = ((sample_rate >> 16) & 0xff);
    ptr[3] = ((sample_rate >> 24) & 0xff);
    ptr += 4;

    // ByteRate
    ptr[0] = ((sample_size >>  0) & 0xff);
    ptr[1] = ((sample_size >>  8) & 0xff);
    ptr[2] = ((sample_size >> 16) & 0xff);
    ptr[3] = ((sample_size >> 24) & 0xff);
    ptr += 4;

    // BlockAlign
    ptr[0] = ((sample_byte >>  0) & 0xff);
    ptr[1] = ((sample_byte >>  8) & 0xff);
    // BitsPerSample
    ptr[2] = ((bitwidth >> 0) & 0xff);
    ptr[3] = ((bitwidth >> 8) & 0xff);
    ptr += 4;

    // Subchunk2ID
    ptr[0] = 'd'; ptr[1] = 'a'; ptr[2] = 't'; ptr[3] = 'a';
    ptr += 4;

    // Subchunk2Size
    ptr[0] = ((voice_data_size >>  0) & 0xff);
    ptr[1] = ((voice_data_size >>  8) & 0xff);
    ptr[2] = ((voice_data_size >>  16) & 0xff);
    ptr[3] = ((voice_data_size >>  24) & 0xff);
}

int app_vc_cloud_data_read(int handle, unsigned char** data, unsigned int *read_real_size)
{
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    if((0 > handle) || (DATA_GET_MAX < handle) || (handle != thiz_handle_control[handle -1].id)
            || (NULL == data)
            || (NULL == read_real_size))
    {
        GxCore_MutexUnlock(thiz_data_mutex);
        DATA_FRINT("\n Parameters error...\n");
        return -1;
    }

    if((VC_CLOUD_IDLE != thiz_vc_data_status)
            || (1 != thiz_handle_control[handle -1].active_flag))
    {
        GxCore_MutexUnlock(thiz_data_mutex);
        DATA_FRINT("\n Status error...\n");
        return -1;
    }

    if((0 > thiz_data_size) || (NULL == thiz_data_cache))
    {
        GxCore_MutexUnlock(thiz_data_mutex);
        DATA_FRINT("\n No data...\n");
        return 1;
    }
    if(0 == thiz_data_size)
    {
        *read_real_size = 0;
        GxCore_MutexUnlock(thiz_data_mutex);
        return 0;
    }
    // TODO: test for PCM data
    *data = GxCore_Malloc(thiz_data_size + 44);
    if(NULL == *data)
    {
        GxCore_MutexUnlock(thiz_data_mutex);
        DATA_FRINT("\n Malloc failed...\n");
        return -1;
    }
    _fill_wave_header(*data, 16, 1, 16000, thiz_data_size);// generate .wav header, 44 Bytes
    memcpy((*data + 44), thiz_data_cache, thiz_data_size);
    *read_real_size = thiz_data_size + 44;
    thiz_vc_data_status = VC_CLOUD_NONE;
    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
}

// 1: begin
int app_vc_cloud_data_is_begin(void)
{
    int ret = 0;
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    ret = ((thiz_vc_data_status == VC_CLOUD_WORKING)?1:0);
    GxCore_MutexUnlock(thiz_data_mutex);
    return ret;
}

// 1: end
int app_vc_cloud_data_is_end(void)
{
    int ret = 0;
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    ret = ((thiz_vc_data_status == VC_CLOUD_IDLE)?1:0);
    GxCore_MutexUnlock(thiz_data_mutex);
    return ret;
}

int app_vc_cloud_data_term(void)
{
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    thiz_vc_data_status = VC_CLOUD_IDLE;
    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
}

int app_vc_cloud_data_begin(void)
{
    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    thiz_vc_data_status = VC_CLOUD_WORKING;
    // TODO:
    // release data
    if(thiz_data_cache)
    {
        GxCore_Free(thiz_data_cache);
        thiz_data_cache = NULL;
        thiz_data_cache_size = DATA_CACHE_BASIC_SIZE;
        thiz_data_size = 0;
    }
    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
}

// from dongle

int app_vc_cloud_voice_data_store(char* data, unsigned int len)
{
    int status = 0;
    int cache_size = 0;

    if(0 == thiz_init_flag)
        return -1;

    GxCore_MutexLock(thiz_data_mutex);
    cache_size = thiz_data_cache_size;
    status = thiz_vc_data_status;

    if(VC_CLOUD_WORKING != status)
    {
        goto err;
    }

    if(DATA_CACHE_MAX_SZIE < (thiz_data_size + len))
    {
        DATA_FRINT("\nOver Flow...\n");
        goto err;
    }

    while((thiz_data_size + len) > cache_size)
    {
        cache_size += DATA_CACHE_BASIC_SIZE;
    }
    if(NULL == thiz_data_cache)
    {
        thiz_data_cache = GxCore_Malloc(cache_size);
        if(NULL == thiz_data_cache)
        {
            DATA_FRINT("\nMalloc failed...\n");
            goto err;
        }
    }
    else if(cache_size != thiz_data_cache_size)
    {
        thiz_data_cache = GxCore_Realloc(thiz_data_cache, cache_size);
        if(NULL == thiz_data_cache)
        {
            DATA_FRINT("\nRealloc failed...\n");
            goto err;
        }
    }
    memcpy(thiz_data_cache+thiz_data_size, data, len);
    thiz_data_size += len;
    thiz_data_cache_size = cache_size;

    GxCore_MutexUnlock(thiz_data_mutex);
    return 0;
err:
    GxCore_MutexUnlock(thiz_data_mutex);
    return -1;
}


