#include "gx_common.h"
#include "audio_resample.h"

#define FILTER_SHIFT 15

#define FELEM  int16_t
#define FELEM2 int32_t
#define FELEML int64_t
#define FELEM_MAX (32767)
#define FELEM_MIN (-32768)
#define WINDOW_TYPE 9

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))

#define MAX_CHANNELS (8)
#define MAX_CHPOINTS (3072)
typedef struct GXResampleContext{
	FELEM         *filter_bank;
	int            filter_length;
	int            dst_incr;
	int            index;
	int            frac;
	int            src_incr;
	int            phase_shift;
	int            phase_mask;
	unsigned char *phase_saved;
	unsigned char *ophase;
	short         *iphase[MAX_CHANNELS];
	short         *tphase;
	int            res_point;
	int            isample_rate;
	int            osample_rate;
	int            ichannels;
	int            ochannels;
	int            ibigendian;
	int            obigendian;
} GXResampleContext;

static short *_resample_filter_addr = NULL;

static int _resample_done(GXResampleContext *ctx, short *src, short *dst, int src_size, int dst_size, int *used_size)
{
	int dst_index, i;
	int index = ctx->index;
	int frac  = ctx->frac;
	int dst_incr_frac = ctx->dst_incr % ctx->src_incr;
	int dst_incr      = ctx->dst_incr / ctx->src_incr;

	for(dst_index=0; dst_index < dst_size; dst_index++) {
		FELEM *filter = ctx->filter_bank + ctx->filter_length*(index & ctx->phase_mask);
		int sample_index = index >> ctx->phase_shift;
		FELEM2 val = 0;

		if (sample_index < 0) {
			for (i=0; i<ctx->filter_length; i++) {
				val += src[FFABS(sample_index + i) % src_size] * filter[i];
			}
		}
		else if (sample_index + ctx->filter_length > src_size) {
			break;
		}
		else {
			for (i=0; i<ctx->filter_length; i++) {
				val += src[sample_index + i] * (FELEM2)filter[i];
			}
		}

		val = (val + (1<<(FILTER_SHIFT-1))) >> FILTER_SHIFT;

		dst[dst_index] = (unsigned)(val + 32768) > 65535 ? (val>>31) ^ 32767 : val;

		frac  += dst_incr_frac;
		index += dst_incr;

		if(frac >= ctx->src_incr) {
			frac -= ctx->src_incr;
			index++;
		}
	}

	*used_size = FFMAX(index, 0) >> ctx->phase_shift;
	if(index >= 0) {
		index &= ctx->phase_mask;
	}

	ctx->frac     = frac;
	ctx->index    = index;
	ctx->dst_incr = dst_incr_frac + ctx->src_incr*dst_incr;

	return dst_index;
}

void gx_resample_register(void)
{
	extern short _resample_filter_all_buf[1024*16];
	_resample_filter_addr = (short *)_resample_filter_all_buf;
}

void *gx_resample_alloc(int isample_rate,
		int ichannels, int osample_rate, int ochannels, int ibigendian, int obigendian)
{
	int phase_count= 1024; //1<<10 = 1024
	GXResampleContext *ctx = NULL;
	unsigned int i = 0;

	if (_resample_filter_addr == NULL) {
		gxloge("%s %d: malloc fail\n", __func__, __LINE__);
		return NULL;
	}

	ctx = (GXResampleContext *)av_mallocz(sizeof(GXResampleContext));
	if (ctx == NULL) {
		gxloge("%s %d: malloc fail\n", __func__, __LINE__);
		return NULL;
	}
	ctx->phase_saved = (unsigned char *)av_mallocz((ichannels + ochannels + 1) * MAX_CHPOINTS * sizeof(short));
	if (ctx->phase_saved == NULL) {
		gxloge("%s %d: malloc fail\n", __func__, __LINE__);
		if (ctx->phase_saved)
			av_free(ctx->phase_saved);
		av_free(ctx);
		return NULL;
	}

	ctx->filter_length = 16;
	ctx->filter_bank  = _resample_filter_addr;
	ctx->ichannels    = ichannels;
	ctx->ochannels    = ochannels;
	ctx->ibigendian   = ibigendian;
	ctx->obigendian   = obigendian;
	ctx->isample_rate = isample_rate;
	ctx->osample_rate = osample_rate;
	ctx->phase_shift = 10; //10
	ctx->phase_mask  = 1023; // 1023
	for (i = 0; i < ichannels; i++)
		ctx->iphase[i] = (short *)(ctx->phase_saved + (i * MAX_CHPOINTS * sizeof(short)));
	ctx->ophase = (ctx->phase_saved + (MAX_CHPOINTS * sizeof(short) *ichannels));
	ctx->tphase = (short *)(ctx->phase_saved + (MAX_CHPOINTS * sizeof(short) *(ichannels + ochannels)));

	ctx->src_incr = osample_rate;
	ctx->dst_incr = isample_rate * phase_count;
	ctx->index= -phase_count*((ctx->filter_length-1)/2);

	return (void *)ctx;
}

int gx_resample(void *handle, GXAudioReSampleParam *para)
{
	unsigned int i = 0;
	unsigned int channels = 1;
	GXResampleContext *ctx = (GXResampleContext *)handle;
	int use_point = 0;
	int res_point = 0;
	int src_point = ((para->src_len / sizeof(short) / ctx->ichannels) + ctx->res_point);
	int dst_point = (src_point * ctx->osample_rate) / ctx->isample_rate + 128;

	if (src_point > MAX_CHPOINTS) {
		gxloge("%s %d: src_point %d > %d\n", __func__, __LINE__, src_point, MAX_CHPOINTS);
		return - 1;
	}

	if (dst_point > MAX_CHPOINTS) {
		gxloge("%s %d: dst_point %d > %d\n", __func__, __LINE__, dst_point, MAX_CHPOINTS);
		return - 1;
	}

	for (i = 0; i < channels; i++) {
		short *src_buf = ctx->iphase[i];
		short *tmp_buf = ctx->tphase;
		unsigned char *dst_buf = ctx->ophase;
		unsigned int j = 0;

		for (j = ctx->res_point; j < src_point; j++) {
			unsigned int pos = (i + (j - ctx->res_point) * ctx->ichannels) * sizeof(short);
			if (ctx->ibigendian)
				src_buf[j] = ((para->src_buf[pos + 1] << 8) | para->src_buf[pos + 0]);
			else
				src_buf[j] = (para->src_buf[pos + 1] | (para->src_buf[pos + 0] << 8));
		}
		dst_point = _resample_done(ctx, src_buf, tmp_buf, src_point, dst_point, &use_point);
		res_point = src_point - use_point;
		for (j = 0; j < res_point; j++)
			src_buf[j] = src_buf[use_point + j];
		for (j = 0; j < dst_point; j++) {
			unsigned int k = 0;
			for (k = 0; k < ctx->ochannels; k++) {
				unsigned int pos = (j * ctx->ochannels + k) * sizeof(short);
				if (ctx->obigendian) {
					dst_buf[pos + 0] = ((tmp_buf[j]) & 0xff);
					dst_buf[pos + 1] = ((tmp_buf[j] >> 8) & 0xff);
				} else {
					dst_buf[pos + 0] = ((tmp_buf[j] >> 8) & 0xff);
					dst_buf[pos + 1] = ((tmp_buf[j]) & 0xff);
				}
			}
		}
	}
	ctx->res_point = res_point;
	para->dst_len = (dst_point * ctx->ochannels * sizeof(short));
	para->dst_buf = ctx->ophase;

	return (dst_point * sizeof(short));
}

void gx_resample_free(void *handle)
{
	GXResampleContext *ctx = (GXResampleContext *)handle;

	if (ctx) {
		if (ctx->phase_saved) {
			av_free(ctx->phase_saved);
			ctx->phase_saved = NULL;
		}
		ctx->filter_bank = NULL;
		av_free(ctx);
	}

	return;
}
