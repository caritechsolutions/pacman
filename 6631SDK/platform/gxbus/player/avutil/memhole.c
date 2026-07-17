#include "gx_system.h"
#include "gx_device.h"
#include "gx_mediafilter.h"
#include "devapi/chip_info.h"

#define AV_MEMHOLE_MAX (80)

static struct memhole_block{
	unsigned int used;
	unsigned int flag;
	unsigned int freeed;
	unsigned long size;
	void* ptr;
} av_memhole_cache[AV_MEMHOLE_MAX];

static unsigned int av_resident_flag[AV_MEMHOLE_MAX];
static unsigned int av_security_flag = 0;

static struct {
	unsigned int now_used;
	unsigned int max_used;
} av_memhole_sata;

static int32_t g_mutex;

static struct {
	struct gxav_memhole_info esa;
	struct gxav_memhole_info esv;
	struct gxav_memhole_info wka;
	struct gxav_memhole_info wkv;
	struct gxav_memhole_info pcm;
	struct gxav_memhole_info vpu;
	struct gxav_memhole_info vfb;
	struct gxav_memhole_info afb;
	struct gxav_memhole_info tsa;
	struct gxav_memhole_info tsw;
} av_cmdline_memhole;

static void av_resident_flag_init(void)
{
	int i = 0;

	memset(&av_resident_flag, 0, sizeof(av_resident_flag));

	if (av_security_flag & GXMEM_FLAG_DEMUX_ES) {
		av_resident_flag[i++] = GX_PINFLAG_ESV;
		av_resident_flag[i++] = GX_PINFLAG_ESA;
	}

	// For AD
	if (av_security_flag & GXMEM_FLAG_DEMUX_TSW) {
		av_resident_flag[i++] = GX_PINFLAG_TSA;
	}

	if (av_security_flag & GXMEM_FLAG_AUDIO_FIRMWARE) {
		av_resident_flag[i++] = GX_PINFLAG_WKA;
	}

	if (av_security_flag & GXMEM_FLAG_AUDIO_FRAME) {
		av_resident_flag[i++] = GX_PINFLAG_PCM;
		av_resident_flag[i++] = GX_PINFLAG_PCM1;
		av_resident_flag[i++] = GX_PINFLAG_AC3;
		av_resident_flag[i++] = GX_PINFLAG_EAC3;
		av_resident_flag[i++] = GX_PINFLAG_DTS;
		av_resident_flag[i++] = GX_PINFLAG_AAC;
	}

	if (av_security_flag & GXMEM_FLAG_VIDEO_FIRMWARE) {
		av_resident_flag[i++] = GX_PINFLAG_WKV;
	}

	if (av_security_flag & GXMEM_FLAG_VIDEO_FRAME) {
		av_resident_flag[i++] = GX_PINFLAG_FBV;
		av_resident_flag[i++] = GX_PINFLAG_FBVZ;
		av_resident_flag[i++] = GX_PINFLAG_FCOL;
	}

	if (av_security_flag & GXMEM_FLAG_VPU_CAP) {
		av_resident_flag[i++] = GX_PINFLAG_SVPU;
	}
}

static void av_cmdline_memhole_init(void)
{
	int av_device = GxAvdev_CreateDevice(0);

	memset(&av_cmdline_memhole, 0, sizeof(av_cmdline_memhole));

	strncpy(av_cmdline_memhole.esa.name, "esamem", 8);
	strncpy(av_cmdline_memhole.esv.name, "esvmem", 8);
	strncpy(av_cmdline_memhole.wka.name, "afwmem", 8);
	strncpy(av_cmdline_memhole.wkv.name, "vfwmem", 8);
	strncpy(av_cmdline_memhole.pcm.name, "pcmmem", 8);
	strncpy(av_cmdline_memhole.vpu.name, "svpumem", 8);
	strncpy(av_cmdline_memhole.vfb.name, "vfbmem", 8);
	strncpy(av_cmdline_memhole.afb.name, "afbmem", 8);
	strncpy(av_cmdline_memhole.tsa.name, "tsamem", 8);
	strncpy(av_cmdline_memhole.tsw.name, "tswmem", 8);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.esa);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.esv);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.wka);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.wkv);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.pcm);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.vpu);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.vfb);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.afb);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.tsa);
	GxAVMemHoleGetInfo(av_device, &av_cmdline_memhole.tsw);

	if (av_cmdline_memhole.esa.start != 0 && av_cmdline_memhole.esa.size > 0) {
		gxlogi("Modify default esa size -> 0x%x\n", av_cmdline_memhole.esa.size);
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_ESA_SIZE, av_cmdline_memhole.esa.size);
		GxDevice_Set(PDEV_BufSizeESA, &av_cmdline_memhole.esa.size);
	}

	if (av_cmdline_memhole.esv.start != 0 && av_cmdline_memhole.esv.size > 0) {
		gxlogi("Modify default esv size -> 0x%x\n", av_cmdline_memhole.esv.size);
		GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_ESV_SIZE, av_cmdline_memhole.esv.size);
		GxDevice_Set(PDEV_BufSizeESV, &av_cmdline_memhole.esv.size);
	}

	if (av_cmdline_memhole.pcm.start != 0 && av_cmdline_memhole.pcm.size > 0) {
		gxlogi("Modify default pcm size -> 0x%x\n", av_cmdline_memhole.pcm.size);
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_PCM_SIZE, av_cmdline_memhole.pcm.size);
		GxDevice_Set(PDEV_BufSizePCM, &av_cmdline_memhole.pcm.size);
	}

	if (av_cmdline_memhole.vfb.start != 0 && av_cmdline_memhole.vfb.size > 0) {
		gxlogi("Init default vfb buffer -> 0x%x, size = 0x%x\n", av_cmdline_memhole.vfb.start, av_cmdline_memhole.vfb.size);
		av_mmv_init(av_cmdline_memhole.vfb.start, av_cmdline_memhole.vfb.size);
	}
	else if (av_security_flag & GXMEM_FLAG_VIDEO_FRAME) {
		while(1)
			gxloge("Video Frame Protect Enable, But no vfbmem configured in cmdline!\n");
	}

	if (av_cmdline_memhole.afb.start != 0 && av_cmdline_memhole.afb.size > 0) {
		gxlogi("Init default afb buffer -> 0x%x, size = 0x%x\n", av_cmdline_memhole.afb.start, av_cmdline_memhole.afb.size);
		av_mma_init(av_cmdline_memhole.afb.start, av_cmdline_memhole.afb.size);
	}
	else if (av_security_flag & GXMEM_FLAG_AUDIO_FRAME) {
		while(1)
			gxloge("Audio Frame Protect Enable, But no afbmem configured in cmdline!\n");
	}

	if (av_cmdline_memhole.tsa.start != 0 && av_cmdline_memhole.tsa.size > 0) {
		gxlogi("Init default tsa buffer -> 0x%x, size = 0x%x\n", av_cmdline_memhole.tsa.start, av_cmdline_memhole.tsa.size);
	}
	else if (av_security_flag & GXMEM_FLAG_DEMUX_TSW) {
		int ad_en = 0;
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AD_ENABLE, &ad_en, 0);
		while(1 && ad_en)
			gxloge("Demux TSW Protect Enable, But no tsamem configured in cmdline!\n");
	}
}

static unsigned char *_alloc_cmdline_memhole(unsigned long ssize, unsigned int flag)
{
#define CHECK_CMDLINE_MEMHOLE(info, ssize, flag) \
	if (info.start) { \
		if (info.size >= ssize) \
			return (unsigned char *)info.start; \
		else \
			gxloge("[Player]:%s, flag=0x%x, size=%ld, ksize=%d\n", __func__, flag, ssize, info.size); \
	} \
	break;

	switch (flag) {
	case GX_PINFLAG_ESV :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.esv, ssize, flag);
	case GX_PINFLAG_ESA :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.esa, ssize, flag);
	case GX_PINFLAG_WKV :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.wkv, ssize, flag);
	case GX_PINFLAG_WKA :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.wka, ssize, flag);
	case GX_PINFLAG_PCM :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.pcm, ssize, flag);
	case GX_PINFLAG_SVPU :
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.vpu, ssize, flag);
	case GX_PINFLAG_TSA:
		CHECK_CMDLINE_MEMHOLE(av_cmdline_memhole.tsa, ssize, flag);
	default:
		break;
	}

	// 单独的 Buffer 没申请到，音视频帧存尝试从 vfb、afb 中申请；
	switch (flag) {
		case GX_PINFLAG_PCM:
		case GX_PINFLAG_PCM1:
		case GX_PINFLAG_AC3:
		case GX_PINFLAG_EAC3:
		case GX_PINFLAG_DTS:
		case GX_PINFLAG_AAC:
			return av_mma_malloc(ssize, flag);
		case GX_PINFLAG_FBV:
		case GX_PINFLAG_FBVZ:
		case GX_PINFLAG_FCOL:
			return av_mmv_malloc(ssize, flag);
		default:
			return NULL;
	}

	return NULL;
}

static int _free_cmdline_memhole(unsigned int addr, unsigned int flag)
{
	switch (flag) {
	case GX_PINFLAG_ESV :
		if (av_cmdline_memhole.esv.start == addr) return 0;
		break;
	case GX_PINFLAG_ESA :
		if (av_cmdline_memhole.esa.start == addr) return 0;
		break;
	case GX_PINFLAG_WKV :
		if (av_cmdline_memhole.wkv.start == addr) return 0;
		break;
	case GX_PINFLAG_WKA :
		if (av_cmdline_memhole.wka.start == addr) return 0;
		break;
	case GX_PINFLAG_PCM :
		if (av_cmdline_memhole.pcm.start == addr) return 0;
		break;
	case GX_PINFLAG_SVPU :
		if (av_cmdline_memhole.vpu.start == addr) return 0;
		break;
	default:
		break;
	}

	// 单独的 Buffer 没找到，音视频帧存尝试从 vfbmem、afbmem 中释放；
	switch (flag) {
		case GX_PINFLAG_PCM:
		case GX_PINFLAG_PCM1:
		case GX_PINFLAG_AC3:
		case GX_PINFLAG_EAC3:
		case GX_PINFLAG_DTS:
		case GX_PINFLAG_AAC:
			return av_mma_free((unsigned char *)addr, flag);
		case GX_PINFLAG_FBV:
		case GX_PINFLAG_FBVZ:
		case GX_PINFLAG_FCOL:
			return av_mmv_free((unsigned char *)addr, flag);
		default:
			return -1;
	}

	return -1;
}

void av_memhole_init(void)
{
	memset(&av_memhole_cache, 0, sizeof(av_memhole_cache));
	memset(&av_memhole_sata, 0, sizeof(av_memhole_sata));

	GxCore_MutexCreate(&g_mutex);

	av_security_flag = GxAvdev_GetFirewallFlag();
	gxlogi("[Player] %s: firewall_flag = %d\n", __func__, av_security_flag);

	av_resident_flag_init();
	av_cmdline_memhole_init();
}

unsigned int av_memhole_tsw_getsize(void)
{
	return av_cmdline_memhole.tsw.size;
}

static void av_memhole_cache_add(void* ptr, unsigned long size, unsigned int flag)
{
	int i;

	for(i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].used == 0) {
			av_memhole_cache[i].used = 1;
			av_memhole_cache[i].freeed = 0;
			av_memhole_cache[i].size = size;
			av_memhole_cache[i].flag = flag;
			av_memhole_cache[i].ptr = ptr;

			av_memhole_sata.now_used += size;
			if (av_memhole_sata.max_used < av_memhole_sata.now_used)
				av_memhole_sata.max_used = av_memhole_sata.now_used;

			return;
		}
	}

	gxloge("%s, cache full error !\n",__func__);

	for(i=0; i<AV_MEMHOLE_MAX; i++) {
		gxloge("[%d]: flags = 0x%x, addr = 0x%p, size = 0x%x\n", i, av_memhole_cache[i].flag, av_memhole_cache[i].ptr, av_memhole_cache[i].size);
	}
}

static struct memhole_block* av_memhole_cache_find_size(unsigned long size, unsigned int flag)
{
	int i;

	for(i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].used == 1 &&
				av_memhole_cache[i].size == size &&
				av_memhole_cache[i].flag == flag &&
				av_memhole_cache[i].freeed == 1) {
			return &av_memhole_cache[i];
		}
	}

	return NULL;
}

static struct memhole_block* av_memhole_cache_find_ptr(void* p, unsigned int flag)
{
	int i;

	for(i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].used == 1 &&
				av_memhole_cache[i].flag == flag &&
				av_memhole_cache[i].ptr == p) {
			return &av_memhole_cache[i];
		}
	}

	return NULL;
}

unsigned char *av_memhole_malloc(unsigned long size, unsigned int flag)
{
	unsigned char *ptr = NULL;

	GxCore_MutexLock(g_mutex);
	ptr = _alloc_cmdline_memhole(size, flag);
	if (ptr != NULL) {
		GxCore_MutexUnlock(g_mutex);
		return ptr;
	}
	else {
		struct memhole_block* mb = av_memhole_cache_find_size(size, flag);
		if (mb) {
			mb->freeed = 0;
			GxCore_MutexUnlock(g_mutex);
			return mb->ptr;
		}
		else {
			ptr = GxCore_MemholeMalloc(size, NULL);
			if (ptr)
				av_memhole_cache_add(ptr, size, flag);
		}
	}
	GxCore_MutexUnlock(g_mutex);

	return ptr;
}

void av_memhole_free(unsigned char *p, unsigned int flag)
{
	GxCore_MutexLock(g_mutex);
	if (_free_cmdline_memhole((unsigned int)p, flag) == 0) {
		GxCore_MutexUnlock(g_mutex);
		return;
	}
	else {
		struct memhole_block* mb = av_memhole_cache_find_ptr(p, flag);
		if (mb) {
			mb->freeed = 1;
			if (mb->flag == GX_PINFLAG_SW) {
				GxCore_MemholeFree(mb->ptr);
				av_memhole_sata.now_used -= mb->size;
				memset(mb, 0, sizeof(struct memhole_block));
			}
		}
		else
			gxlogd("%s, free NULL\n",__func__);
	}
	GxCore_MutexUnlock(g_mutex);
}

unsigned char *av_memhole_malloc_user(unsigned long size)
{
	unsigned char *user_ptr = NULL;
	unsigned char *kernel_ptr = GxCore_MemholeMalloc(size, &user_ptr);

	if (!kernel_ptr)
		return NULL;

	av_memhole_cache_add(kernel_ptr, size, GX_PINFLAG_SW);

	gxlogf("%s %d %p\n", __FUNCTION__, __LINE__, kernel_ptr);
	return user_ptr;
}

void av_memhole_free_user(unsigned char *p)
{
	if (p) {
		unsigned char *kernel_ptr = GxCore_MemholeMap(p);
		av_memhole_free(kernel_ptr, GX_PINFLAG_SW);
	}
}

unsigned char *av_memhole_malloc_kernel(unsigned long size)
{
	unsigned char *kernel_ptr = GxCore_MemholeMalloc(size, NULL);

	if (!kernel_ptr)
		return NULL;

	av_memhole_cache_add(kernel_ptr, size, GX_PINFLAG_SW);

	gxlogf("%s %d %p\n", __FUNCTION__, __LINE__, kernel_ptr);
	return kernel_ptr;
}

void av_memhole_free_kernel(unsigned char *p)
{
	if (p) {
		av_memhole_free(p, GX_PINFLAG_SW);
	}
}

unsigned int av_memhole_now_used(void)
{
	return av_memhole_sata.now_used;
}

unsigned int av_memhole_max_used(void)
{
	return av_memhole_sata.max_used;
}

static int av_memhole_resident(unsigned int flag)
{
	switch (flag) {
	case GX_PINFLAG_ESV :
	case GX_PINFLAG_ESA :
	case GX_PINFLAG_PCM :
	case GX_PINFLAG_TSA :
		return 1;
	default:
		{
			int i;
			for (i=0; i<AV_MEMHOLE_MAX; i++) {
				if (av_resident_flag[i] == 0)
					return 0;
				if (av_resident_flag[i] == flag)
					return 1;
			}
		}
	}

	return 0;
}

void av_memhole_flush(void)
{
	int i;

	GxCore_MutexLock(g_mutex);

	av_mmv_flush();
	av_mma_flush();

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].used == 1 && av_memhole_cache[i].freeed == 1) {
			if (av_memhole_resident(av_memhole_cache[i].flag) == 0) {
				gxlogi("@@@@@ %s, ptr:0x%p, size = %ld, type = 0x%x\n", __func__,
						av_memhole_cache[i].ptr, av_memhole_cache[i].size, av_memhole_cache[i].flag);
				GxCore_MemholeFree(av_memhole_cache[i].ptr);
				av_memhole_sata.now_used -= av_memhole_cache[i].size;
				memset(&av_memhole_cache[i], 0, sizeof(struct memhole_block));
			}
		}
	}
	GxCore_MutexUnlock(g_mutex);
}

void av_memhole_destroy(void)
{
	int i;

	GxCore_MutexLock(g_mutex);

	av_mmv_flush();
	av_mma_flush();

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].used == 1 && av_memhole_cache[i].freeed == 1) {
			gxlogi("@@@@@ %s, ptr:0x%p, size = %ld, type = 0x%x\n", __func__,
					av_memhole_cache[i].ptr, av_memhole_cache[i].size, av_memhole_cache[i].flag);
			GxCore_MemholeFree(av_memhole_cache[i].ptr);
			av_memhole_sata.now_used -= av_memhole_cache[i].size;
			memset(&av_memhole_cache[i], 0, sizeof(struct memhole_block));
		}
	}
	GxCore_MutexUnlock(g_mutex);
}


void av_memhole_print()
{
	int i;

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole_cache[i].ptr)
			gxlogi("@@@@@ %s, ptr:0x%p, size = %ld, type = 0x%x, used = %d, freeed = %d\n", __func__,
				av_memhole_cache[i].ptr, av_memhole_cache[i].size, av_memhole_cache[i].flag, av_memhole_cache[i].used, av_memhole_cache[i].freeed);
	}
}
