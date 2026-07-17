#ifndef __AUDIUO_RESAMPLE_H__
#define __AUDIUO_RESAMPLE_H__

typedef struct {
	unsigned char *src_buf;
	unsigned int   src_len;
	unsigned char *dst_buf;
	unsigned int   dst_len;
} GXAudioReSampleParam;

void  gx_resample_register(void);
void *gx_resample_alloc(int isample_rate,
		int ichannels, int osample_rate, int ochannels, int ibigendian, int obigendian);
int   gx_resample(void *handle, GXAudioReSampleParam *para);
void  gx_resample_free(void *handle);

#endif
