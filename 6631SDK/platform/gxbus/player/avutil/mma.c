#include "gx_system.h"
#include "gx_mediafilter.h"
#include "gx_device.h"

enum {
	MMA_PCM = 0,
	MMA_PCM1,
	MMA_AC3,
	MMA_EAC3,
	MMA_DTS,
	MMA_AAC,
	MMA_MAX
};

static struct {
	int inited;
	unsigned long addr, size;
	struct {
		unsigned char* start;
		unsigned long  size;
		unsigned int   flag;
		int            enable;
	} bank[MMA_MAX];
} mma_info = {.inited = 0, };

void av_mma_init(unsigned long start, unsigned long size)
{
	int i;
	unsigned char* naddr = (unsigned char *)start;
	unsigned long  nsize = size;

	memset(&mma_info, 0, sizeof(mma_info));


	mma_info.bank[MMA_PCM].enable = 1;
	GxDevice_Get(PDEV_EnableADPCM, &mma_info.bank[MMA_PCM1].enable);
	GxDevice_Get(PDEV_EnableAc3,   &mma_info.bank[MMA_AC3].enable);
	GxDevice_Get(PDEV_EnableAc3,   &mma_info.bank[MMA_EAC3].enable);
	GxDevice_Get(PDEV_EnableDts,   &mma_info.bank[MMA_DTS].enable);
	GxDevice_Get(PDEV_EnableAac,   &mma_info.bank[MMA_AAC].enable);

	GxDevice_Get(PDEV_BufSizePCM,  &mma_info.bank[MMA_PCM].size);
	GxDevice_Get(PDEV_BufSizePCM,  &mma_info.bank[MMA_PCM1].size);
	GxDevice_Get(PDEV_BufSizeAC3,  &mma_info.bank[MMA_AC3].size);
	GxDevice_Get(PDEV_BufSizeEAC3, &mma_info.bank[MMA_EAC3].size);
	GxDevice_Get(PDEV_BufSizeDTS,  &mma_info.bank[MMA_DTS].size);
	GxDevice_Get(PDEV_BufSizeAAC,  &mma_info.bank[MMA_AAC].size);

	gxlogi("------------------------------------------------------------------------\n");
	for (i=0; i<MMA_MAX; i++)
		gxlogi("%s:[%d]: enable = %d, bufsize = %ld\n", __func__, i, mma_info.bank[i].enable, mma_info.bank[i].size);
	gxlogi("------------------------------------------------------------------------\n");

	mma_info.bank[MMA_PCM].flag  = GX_PINFLAG_PCM;
	mma_info.bank[MMA_PCM1].flag = GX_PINFLAG_PCM1;
	mma_info.bank[MMA_AC3].flag  = GX_PINFLAG_AC3;
	mma_info.bank[MMA_EAC3].flag = GX_PINFLAG_EAC3;
	mma_info.bank[MMA_DTS].flag  = GX_PINFLAG_DTS;
	mma_info.bank[MMA_AAC].flag  = GX_PINFLAG_AAC;

	for (i=0; i<MMA_MAX; i++) {
		if (mma_info.bank[i].enable == 1) {
			if (nsize < mma_info.bank[i].size) {
				gxloge("%s():%d, size = %ld\n not enough\n", __func__, __LINE__, size);
				return;
			}
			mma_info.bank[i].start = naddr;
			naddr += mma_info.bank[i].size;
			nsize -= mma_info.bank[i].size;
		}
	}

	mma_info.addr = start;
	mma_info.size = size;
	mma_info.inited = 1;
}

static int _flag2index(unsigned flag)
{
	switch (flag) {
		case GX_PINFLAG_PCM:  return MMA_PCM;
		case GX_PINFLAG_PCM1: return MMA_PCM1;
		case GX_PINFLAG_AC3:  return MMA_AC3;
		case GX_PINFLAG_EAC3: return MMA_EAC3;
		case GX_PINFLAG_DTS:  return MMA_DTS;
		case GX_PINFLAG_AAC:  return MMA_AAC;
		default:
			return MMA_MAX;
	}
}

unsigned char *av_mma_malloc(unsigned long size, unsigned int flag)
{
	int i = _flag2index(flag);

	if (mma_info.inited == 0 || i == MMA_MAX)
		return NULL;

	if (size == mma_info.bank[i].size)
		return mma_info.bank[i].start;
	else
		gxloge("%s():%d, flag = 0x%x, size = 0x%x not match (0x%x)\n", __func__, __LINE__, flag, size, mma_info.bank[i].size);

	return NULL;
}

int av_mma_free(unsigned char *p, unsigned int flag)
{
	if (mma_info.inited == 0)
		return -1;

	switch (flag) {
		case GX_PINFLAG_PCM:
		case GX_PINFLAG_PCM1:
		case GX_PINFLAG_AC3:
		case GX_PINFLAG_EAC3:
		case GX_PINFLAG_DTS:
		case GX_PINFLAG_AAC:
			return 0;
		default:
			return -1;
	}
}

void av_mma_flush(void)
{

}
