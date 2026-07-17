#include <av/avapi.h>
#include <gxcore.h>
#include "gxavdev.h"
#include <gxsecure/gxsecure_api.h>

#define AVDEV_MAX_MODULE     (GXAV_MOD_MAX)
#define AVDEV_MAX_SUBMOD     (16)
#define AVDEV_MAX_DEMUX      (16)

static int device_handle = -1;
static int device_opr_count = 0;
static handle_t device_mutex = -1;

static handle_t spp_mutex = -1;

static int module_handle[AVDEV_MAX_MODULE][AVDEV_MAX_SUBMOD];
static int module_opr_count[AVDEV_MAX_MODULE][AVDEV_MAX_SUBMOD];
static handle_t module_mutex = -1;

static handle_t demuxsrc_mutex = -1;
static struct demux_source{
	int used;
	int tsid;
	int count;
} demuxsource[AVDEV_MAX_DEMUX];

int GxAvdev_CreateDevice(int pid)
{
	if(device_mutex == -1)
		GxCore_MutexCreate(&device_mutex);

	GxCore_MutexLock(device_mutex);

	if (-1 == device_handle)
		device_handle = GxAVCreateDevice(pid);

	if(device_handle>=0)
		device_opr_count++;

	GxCore_MutexUnlock(device_mutex);

	return device_handle;
}

int GxAvdev_DestroyDevice(int dev_id)
{
	int status = 0;

	GxCore_MutexLock(device_mutex);

	if (1 == device_opr_count) {
		status = GxAVDestroyDevice(dev_id);
		device_handle = -1;
	}
	device_opr_count--;

	GxCore_MutexUnlock(device_mutex);

	return status;
}

int GxAvdev_OpenModule(int dev,int type, int id)
{
	if (module_mutex == -1) {
		GxCore_MutexCreate(&module_mutex);
		memset(module_handle, -1, sizeof(module_handle));
		memset(module_opr_count, 0, sizeof(module_opr_count));
	}

	if (type >= AVDEV_MAX_MODULE || id >= AVDEV_MAX_SUBMOD || id < 0)
		return -1;

	if (type == GXAV_MOD_DVR)
		return GxAVOpenModule(dev,type,id);

	GxCore_MutexLock(module_mutex);

	if (-1 == module_handle[type][id])
		module_handle[type][id] = GxAVOpenModule(dev,type,id);

	if(module_handle[type][id]>=0)
		module_opr_count[type][id]++;

	GxCore_MutexUnlock(module_mutex);

	return module_handle[type][id];
}

int GxAvdev_CloseModule(int dev, int module)
{
	int i,j;
	int status = 0;

	GxCore_MutexLock(module_mutex);

	for (i = 0; i < AVDEV_MAX_MODULE; i++) {
		for (j = 0; j < AVDEV_MAX_SUBMOD; j++) {
			if (module_handle[i][j] == module) {
				if (1 == module_opr_count[i][j]) {
					status = GxAVCloseModule(dev,module);
					module_handle[i][j] = -1;
				}
				module_opr_count[i][j]--;
				goto end;
			}
		}
	}

	if (i == AVDEV_MAX_MODULE)
		status = GxAVCloseModule(dev,module);

end:
	GxCore_MutexUnlock(module_mutex);

	return status;
}

int GxAvdev_CloseLogo(void)
{
	int module;
	int device = GxAvdev_CreateDevice(0);
	module = GxAvdev_OpenModule(device, GXAV_MOD_VPU, 0);

	return GxAvdev_LayerEnable(device, module, GX_LAYER_VPP, 0);
}

int GxAvdev_CloseLogo2(LogoLayerID layer)
{
	int module;
	int device = GxAvdev_CreateDevice(0);
	module = GxAvdev_OpenModule(device, GXAV_MOD_VPU, 0);

	return GxAvdev_LayerEnable(device, module, layer, 0);
}

int GxAvdev_AllocDemux(int tsid)
{
	int i,dmxid=-1;

	// TODO:
	return 0;

	if(demuxsrc_mutex == -1) {
		GxCore_MutexCreate(&demuxsrc_mutex);
		memset(demuxsource,0,sizeof(demuxsource));
	}

	GxCore_MutexLock(demuxsrc_mutex);

	for(i=0;i<AVDEV_MAX_DEMUX;i++) {
		if(demuxsource[i].used == 1 && demuxsource[i].tsid == tsid) {
			dmxid = i;
			demuxsource[i].count++;
			break;
		}
	}

	if(dmxid == -1) {
		for(i=0;i<AVDEV_MAX_DEMUX;i++) {
			if(demuxsource[i].used == 0) {
				demuxsource[i].used = 1;
				demuxsource[i].tsid = tsid;
				demuxsource[i].count = 1;
				dmxid = i;
				break;
			}
		}
	}

	GxCore_MutexUnlock(demuxsrc_mutex);

	return (dmxid);
}

int GxAvdev_FreeDemux(int dmxid)
{
	// TODO:
	return 0;

	if (demuxsrc_mutex == -1)
		return -1;

	if (dmxid >= AVDEV_MAX_DEMUX-1)
		return -1;

	GxCore_MutexLock(demuxsrc_mutex);

	if (demuxsource[dmxid].used) {
		demuxsource[dmxid].count--;
		if (demuxsource[dmxid].count <= 0) {
			demuxsource[dmxid].used = 0;
			demuxsource[dmxid].tsid = -1;
		}
	}

	GxCore_MutexUnlock(demuxsrc_mutex);

	return 0;

}

int GxAvdev_SppLock(void)
{
	if(spp_mutex == -1)
		GxCore_SemCreate(&spp_mutex, 1);

	GxCore_SemWait(spp_mutex);

	return (0);
}

int GxAvdev_SppUnlock(void)
{
	if(spp_mutex == -1)
		GxCore_SemCreate(&spp_mutex, 0);

	GxCore_SemPost(spp_mutex);

	return (0);
}

int GxAvdev_GetLayerEnable(int dev, int vpu, int layer)
{
	GxVpuProperty_LayerEnable property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;
	GxAVGetProperty(dev, vpu,
			GxVpuPropertyID_LayerEnable, (void*)&property, sizeof(GxVpuProperty_LayerEnable));

	return property.enable;
}

int GxAvdev_LayerEnable(int dev, int vpu, int layer, int flag)
{
	GxVpuProperty_LayerEnable property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;
	property.enable = flag ? TRUE : FALSE;

	return GxAVSetProperty(dev,
			vpu,
			GxVpuPropertyID_LayerEnable,
			&property,
			sizeof(GxVpuProperty_LayerEnable));
}

int GxAvdev_SetLayerViewport(int dev, int vpu, int layer, GxAvRect* rect)
{
	GxVpuProperty_LayerViewport property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;
	property.rect = *rect;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_LayerViewport, (void*)&property, sizeof(GxVpuProperty_LayerViewport));
}

int GxAvdev_GetLayerViewport(int dev, int vpu, int layer, GxAvRect* rect)
{
	GxVpuProperty_LayerViewport property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;

	GxAVGetProperty(dev, vpu,
			GxVpuPropertyID_LayerViewport, (void*)&property, sizeof(GxVpuProperty_LayerViewport));
	memcpy(rect, &property.rect, sizeof(GxAvRect));

	return 0;
}

int GxAvdev_SetLayerClipport(int dev, int vpu, int layer, GxAvRect* rect)
{
	GxVpuProperty_LayerClipport property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;
	property.rect = *rect;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_LayerClipport, (void*)&property, sizeof(GxVpuProperty_LayerClipport));
}

int GxAvdev_SetVirtualResolution(int dev, int vpu, int width, int height)
{
	GxVpuProperty_Resolution property;

	memset(&property, 0, sizeof(property));
	property.xres = width;
	property.yres = height;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_VirtualResolution,
			&property,
			sizeof(GxVpuProperty_Resolution));
}

int GxAvdev_SetLayerTop(int dev, int vpu, int layer, int flag)
{
	GxVpuProperty_LayerOnTop property;

	memset(&property, 0, sizeof(property));
	property.layer  = layer;
	property.enable = flag ? 1 : 0;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_LayerOnTop, (void*)&property, sizeof(GxVpuProperty_LayerOnTop));
}

int GxAvdev_GetLayerTop(int dev, int vpu, int layer, int *flag)
{
	GxVpuProperty_LayerOnTop property;

	memset(&property, 0, sizeof(property));
	property.layer  = layer;

	GxAVGetProperty(dev, vpu,
			GxVpuPropertyID_LayerOnTop, (void*)&property, sizeof(GxVpuProperty_LayerOnTop));
	*flag = (property.enable) ? 1 : 0;

	return 0;
}

void* GxAvdev_GetLayerMainSurface(int dev, int vpu, int layer)
{
	GxVpuProperty_LayerMainSurface property;
	int ret = 0;

	memset(&property, 0, sizeof(property));
	property.layer   = layer;

	ret = GxAVGetProperty(dev,
			vpu,
			GxVpuPropertyID_LayerMainSurface,
			(void*)&property,
			sizeof(GxVpuProperty_LayerMainSurface));
	if(ret)
	{
		gxlogd("Get Main Surface failed.\n");
	}
	return (property.surface);
}

int GxAvdev_SetLayerMainSurface(int dev, int vpu, int layer, void *surface)
{
	GxVpuProperty_LayerMainSurface property;

	memset(&property, 0, sizeof(property));
	property.layer   = layer;
	property.surface = surface;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_LayerMainSurface, (void*)&property, sizeof(GxVpuProperty_LayerMainSurface));
}

int GxAvdev_UnSetLayerMainSurface(int dev, int vpu, int layer, void *surface)
{
	GxVpuProperty_LayerMainSurface property;

	memset(&property, 0, sizeof(property));
	property.layer   = layer;
	property.surface = surface;

	return GxAVSetProperty(dev,
			vpu,
			GxVpuPropertyID_UnsetLayerMainSurface,
			(void*)&property,
			sizeof(GxVpuProperty_LayerMainSurface));
}

int GxAvdev_SetSurfaceAlpha(int dev, int vpu, void *surface, GxAlpha *alpha)
{
	GxVpuProperty_Alpha property;

	memset(&property, 0, sizeof(property));
	property.surface = surface;
	memcpy(&property.alpha, alpha, sizeof(GxAlpha));

	return GxAVSetProperty(dev, vpu, GxVpuPropertyID_Alpha, &property, sizeof(GxVpuProperty_Alpha));
}

int GxAvdev_SetLayerAntiFlicker(int dev, int vpu, int layer, int flag)
{
	GxVpuProperty_LayerAntiFlicker property;

	memset(&property, 0, sizeof(property));
	property.layer = layer;
	property.enable = flag ? 1 : 0;

	return GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_LayerAntiFlicker,
			&property, sizeof(GxVpuProperty_LayerAntiFlicker));
}

unsigned int GxAvdev_GetKernelVersion(void)
{
	int major = 0, minor = 0, release = 0;
#ifdef LINUX_OS
#include <sys/utsname.h>
	char *save_p = NULL;
	struct utsname u;
	if (uname(&u) != -1) {
#if 0
		gxlogd("########## KernelVersion: ##########\n"
				"sysname:%s\n"
				"nodename:%s\n"
				"release:%s\n"
				"version:%s\n"
				"machine:%s\n"
				"####################################\n"
				, u.sysname, u.nodename, u.release, u.version, u.machine);
#endif
		major = atoi(strtok_r(u.release, ".", &save_p));
		minor = atoi(strtok_r(NULL, ".", &save_p));
		release = atoi(strtok_r(NULL, "-", &save_p));
	}
#endif
	return KERNEL_VERSION(major, minor, release);
}

int GxAvdev_Memset(void *buffer, char value, unsigned int width, unsigned int height)
{
	int ret;
	static int dev = 0, vpu = 0;
	GxSurface surface;
	GxVpuProperty_FillRect FillRect = {0};
	GxVpuProperty_EndUpdate EndUpdate = {0};
	GxVpuProperty_BeginUpdate BeginUpdate = {0};

	if (dev == 0 || vpu  == 0) {
		dev = GxAvdev_CreateDevice(0);
		vpu = GxAvdev_OpenModule(dev, GXAV_MOD_VPU, 0);
	}

	surface.width  = width;
	surface.height = height;
	surface.buffer = buffer;
	surface.color_format = GX_COLOR_FMT_CLUT8;

	BeginUpdate.max_job_num = 1;
	ret = GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_BeginUpdate,
			(void*)&BeginUpdate,
			sizeof(GxVpuProperty_BeginUpdate));

	FillRect.rect.x = 0;
	FillRect.rect.y = 0;
	FillRect.rect.width = width;
	FillRect.rect.height = height;
	FillRect.is_ksurface = 0;
	FillRect.color.entry = value;
	FillRect.surface = &surface;
	ret |= GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_FillRect,
			(void *)&FillRect,
			sizeof(GxVpuProperty_FillRect));

	ret |= GxAVSetProperty(dev, vpu,
			GxVpuPropertyID_EndUpdate,
			(void*)&EndUpdate,
			sizeof(GxVpuProperty_EndUpdate));

	return 0;
}

unsigned int GxAvdev_GetFirewallFlag(void)
{
	return GxCore_MemProtectFlagGet();
}

