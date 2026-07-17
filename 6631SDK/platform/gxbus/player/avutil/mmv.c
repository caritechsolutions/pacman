#include "gx_system.h"
#include "gx_mediafilter.h"
#include "gx_device.h"

enum {
	MMV_FBVZ = 0,
	MMV_FCOL = 1,
	MMV_FBV = 2,
};

#define MMV_BANK_MAX (16)
static struct {
	int inited;
	unsigned long addr, size;
	struct {
		unsigned char* start;
		unsigned int   size;
		unsigned int   flag;
	} bank[MMV_BANK_MAX];
} mmv_info = {.inited = 0, };

#define MMV_FBV_MAX  (64)
static struct {
	unsigned char* fb_addr[MMV_FBV_MAX];
	unsigned int   fb_size;
	unsigned int   fb_index;
	unsigned int   fb_number;
} mmv_fb;

#define MMV_ALIGN_SIZE       (0x1000)
#define MMV_DO_ALIGN(value)  value = (unsigned int)((value + MMV_ALIGN_SIZE -1) & (~(MMV_ALIGN_SIZE - 1)))

void av_mmv_init(unsigned long start, unsigned long size)
{
	unsigned char* naddr = (unsigned char *)start;
	unsigned long  nsize = size;

	// FBVZ
	mmv_info.bank[MMV_FBVZ].start = naddr;
	GxDevice_Get(PDEV_FreezeFrameSize, &mmv_info.bank[MMV_FBVZ].size);
	MMV_DO_ALIGN(mmv_info.bank[MMV_FBVZ].size);
	nsize -= mmv_info.bank[MMV_FBVZ].size;
	naddr += mmv_info.bank[MMV_FBVZ].size;

	// FCOL
	mmv_info.bank[MMV_FCOL].start = naddr;
	mmv_info.bank[MMV_FCOL].size = mmv_info.bank[MMV_FBVZ].size*2;
	MMV_DO_ALIGN(mmv_info.bank[MMV_FCOL].size);
	nsize -= mmv_info.bank[MMV_FCOL].size;
	naddr += mmv_info.bank[MMV_FCOL].size;

	mmv_info.bank[MMV_FBV].start = naddr;
	mmv_info.bank[MMV_FBV].size  = nsize;

	mmv_info.inited = 1;
	mmv_info.addr = start;
	mmv_info.size = size;

	av_mmv_flush();
}

unsigned char *av_mmv_malloc(unsigned long size, unsigned int flag)
{
	if (mmv_info.inited == 0)
		return NULL;

	if (flag == GX_PINFLAG_FBV) {
		if (mmv_fb.fb_size == 0) {
			int i = 0;
			mmv_fb.fb_size = size;
			MMV_DO_ALIGN(mmv_fb.fb_size);
			mmv_fb.fb_number = mmv_info.bank[MMV_FBV].size / mmv_fb.fb_size;
			mmv_fb.fb_number = mmv_fb.fb_number > MMV_FBV_MAX ? MMV_FBV_MAX : mmv_fb.fb_number;
			for (i = 0; i < mmv_fb.fb_number; i++) {
				mmv_fb.fb_addr[i] = mmv_info.bank[MMV_FBV].start + i * mmv_fb.fb_size;
			}
		}
		if (mmv_fb.fb_index >= mmv_fb.fb_number) {
			gxlogd("...%s:%d..NULL.\n", __func__, __LINE__);
			return NULL;
		}
		return mmv_fb.fb_addr[mmv_fb.fb_index++];
	}
	else if (flag == GX_PINFLAG_FBVZ) {
		return mmv_info.bank[MMV_FBVZ].start;
	}
	else if (flag == GX_PINFLAG_FCOL) {
		return mmv_info.bank[MMV_FCOL].start;
	}

	return NULL;
}

int av_mmv_free(unsigned char *p, unsigned int flag)
{
	if (mmv_info.inited == 0)
		return -1;

	switch (flag) {
		case GX_PINFLAG_FBV:
		case GX_PINFLAG_FBVZ:
		case GX_PINFLAG_FCOL:
			return 0;
		default:
			return -1;
	}
}

void av_mmv_flush(void)
{
	memset(&mmv_fb, 0, sizeof(mmv_fb));
}
