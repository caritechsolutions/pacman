#include "gx_stream.h"
#include "gx_media.h"
#include "dvbhal/gxfe_hal.h"

#define STREAM_FM_SAMPLE_RATE     (48000)
#define STREAM_FM_SAMPLE_CHANNELS (1)
#define STREAM_FM_SAMPLE_ENDIAN   (0)
#define STREAM_FM_SAMPLE_BIT      (16)

typedef struct {
	GxStream parent;
	int      dev;
	int      fre;
	int      handle;
	int      read_exit;
} GxStreamFM;

static int __fedev  = -1;
static int __fecnt  = 0;
static int __felock = 0;
#define MAX_FM_COUNT (16)

static int __check_fedev(void)
{
	if (__fecnt >= MAX_FM_COUNT) {
		gxloge("%s %d: max fe (%d) channel\n", MAX_FM_COUNT);
		return -1;
	}
	return 0;
}

static int __alloc_fedev(int tuner, int fre)
{
	if (__fecnt == 0) {
		GxFeTpParameters tp_params;

		memset(&tp_params, 0, sizeof(GxFeTpParameters));
		tp_params.frequency_KHz = fre;
		tp_params.type          = FE_FM;

		__fedev = GxFeOpen(tuner);
		GxFeAllocateFMBuffer(__fedev);
		GxFeInitFM(__fedev, 128*1024, MAX_FM_COUNT);     //GxFeOpenFMOneChannel个数，实际使用到才分配内存．
		GxFeLockTp(__fedev, tp_params);        //不能lock两次
	}

	__fecnt++;
	return __fedev;
}

static int __free_fedev(int dev)
{
	if (__fecnt > 0) {
		__fecnt--;
		if (__fecnt == 0) {
			GxFeUnlockTp(dev);
			GxFeFreeFMBuffer(dev);
			GxFeClose(dev);
			__fedev = -1;
		}
	}

	return GX_PLAYER_OK;
}

static int fm_open(GxStream* s, int mode)
{
	GxStreamFM *fm = (GxStreamFM *)s;
	int tuner = 0;
	int i     = 0;

	if (__check_fedev() < 0)
		return GX_PLAYER_ERROR;

	GxUrl_GetItem(fm->parent.url, GX_URL_KEY_TUNER, &tuner);
	GxUrl_GetItem(fm->parent.url, GX_URL_KEY_FRE, &fm->fre);

	fm->dev = __alloc_fedev(tuner, fm->fre);
	fm->handle = GxFeOpenFMOneChannel();
	fm->parent.file_format = GX_STREAMTYPE_FM;
	fm->read_exit = 0;

	return GX_PLAYER_OK;
}

static int fm_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
	case GX_STREAM_CTRL_GET_DATA_INFO:
		{
			GxStreamDataInfo *data_info = (GxStreamDataInfo *)arg;

			data_info->samplerate = STREAM_FM_SAMPLE_RATE;
			data_info->channels   = STREAM_FM_SAMPLE_CHANNELS;
			data_info->endian     = STREAM_FM_SAMPLE_ENDIAN;
			data_info->bitwidth   = STREAM_FM_SAMPLE_BIT;
			return GX_PLAYER_OK;
		}
	default:
		break;
	}

	return GX_PLAYER_OK;
}

static int fm_run(GxMediaFilter *filter)
{
	GxStreamFM *fm = (GxStreamFM*)(GXSTREAM(filter));

	fm->read_exit = 0;

	return GX_PLAYER_OK;
}

static int fm_stop(GxMediaFilter *filter)
{
	GxStreamFM *fm = (GxStreamFM*)(GXSTREAM(filter));

	fm->read_exit = 1;

	return GX_PLAYER_OK;
}

static void fm_close(GxStream *s)
{
	GxStreamFM *fm = (GxStreamFM *)s;
	int i  = 0;

	GxFeCloseFMOneChannel(fm->handle);
	__free_fedev(fm->dev);

	return;
}

static int fm_read(GxStream *s, uint8_t *buffer, size_t max_len)
{
	GxStreamFM *fm = (GxStreamFM *)s;
	GxFeDataParams data;
	unsigned int size = 0;

read_again:
	data.buffer   = buffer + size;
	data.read_len = max_len - size;
	data.real_len = 0;
	GxFeReadData(fm->handle, &data);

	if (data.real_len < 0) {
		return -1;
	}

	size += data.real_len;
	if (fm->read_exit) {
		return size;
	}

	if (size < max_len) {
		GxCore_ThreadDelay(10);//解决不同线程优先级导致线程一直被占用问题．
		goto read_again;
	}

	return size;
}

static int fm_config(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int fm_pause(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int fm_resume(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_fm = {
	._inherit = {
		._inherit     = {
			.name    = "StreamFM",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStreamFM),
		},
		.run     = fm_run,
		.stop    = fm_stop,
		.pause   = fm_pause,
		.resume  = fm_resume,
		.config  = fm_config,
	},
	.protocols = {"fm", NULL},
	DEF_AUTHOR("Stream","fm","No description","L.F","No comment"),

	.flags   = 0,
	.open    = fm_open,
	.read    = fm_read,
	.write   = NULL,
	.seek    = NULL,
	.control = fm_control,
	.close   = fm_close,
};
