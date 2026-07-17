
#include "gx_subrender.h"
#include "spudec.h"
#include "../../include/module/player/gxplayer_module.h"
#ifdef LINUX_OS
#include <sys/mman.h>
#endif

#ifndef I386_LINUX

static int              s_device_handle;
static handle_t         s_vpu_handle;
static void*            s_new_surface;
static uint16_t*        s_new_buffer;
static void*            s_raw_surface;
static uint8_t          s_surface_top;
static uint8_t          s_surface_en;
static uint32_t         s_virwidth = 1280;
static uint32_t         s_virheight = 720;
static uint32_t         s_width = 720;
static uint32_t         s_height = 576;
static uint32_t         s_spu_width;
static uint32_t         s_spu_height;

static void inline point_spu(int x0, int y0, int yuv)
{
	int y, u, v;
	uint16_t value;

	if (s_new_buffer == NULL)
		return;

	y = yuv >> 16 & 0xff;
	u = yuv >> 8 & 0xff;
	v = yuv & 0xff;

	value = ((y>>2)<<10 )|((u>>4)<<6)|((v>>4)<<2)|0x3;
	s_new_buffer[s_width*y0+x0] = ((value>>8)&0xff)|((value&0xff)<<8);
}

static inline void clean_spu(void)
{
	int i=0;

	if (s_new_buffer == NULL)
		return;

	for (i=0;i<s_width*s_height;i++)
		s_new_buffer[i] = 0x2002;
}

static inline void hide_spu(void)
{
	GxVpuProperty_LayerEnable       enable;
	
	enable.layer  = GX_LAYER_SPP;
	enable.enable = 0;
	GxAVSetProperty(s_device_handle, s_vpu_handle, 
					GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
	return;
}

static inline void show_spu(void)
{
	GxVpuProperty_LayerEnable       enable;

	enable.layer  = GX_LAYER_SPP;
	enable.enable = 1;
	GxAVSetProperty(s_device_handle, s_vpu_handle,
					GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
	return;
}

static inline void save_spu(int flag)
{
	GxVpuProperty_LayerMainSurface  main_surface;
	GxVpuProperty_LayerOnTop        top;
	GxVpuProperty_LayerEnable       enable;
	GxVpuProperty_Resolution  VirtResolution;
	GxVpuProperty_LayerViewport ViewPort;

	if (flag ==0) {
		main_surface.layer = GX_LAYER_SPP;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerMainSurface, (void*)&main_surface, sizeof(GxVpuProperty_LayerMainSurface));
		s_raw_surface = main_surface.surface;

		top.layer  = GX_LAYER_SPP;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerOnTop, (void*)&top, sizeof(GxVpuProperty_LayerOnTop));
		s_surface_top = top.enable;

		enable.layer  = GX_LAYER_SPP;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
		s_surface_en = enable.enable;

		GxAVGetProperty(s_device_handle,s_vpu_handle,
		      GxVpuPropertyID_VirtualResolution, &VirtResolution, sizeof(GxVpuProperty_Resolution));
		s_virwidth  = VirtResolution.xres;
		s_virheight = VirtResolution.yres;
	} else {
		enable.layer  = GX_LAYER_SPP;
		enable.enable = 0;
		GxAVSetProperty(s_device_handle, s_vpu_handle,
							GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));

		if (s_surface_top == 0) {
			top.layer  = GX_LAYER_SPP;
			top.enable = 0;
			GxAVSetProperty(s_device_handle, s_vpu_handle,
							GxVpuPropertyID_LayerOnTop, (void*)&top, sizeof(GxVpuProperty_LayerOnTop));
		}

		if (s_raw_surface != NULL) {
			main_surface.layer = GX_LAYER_SPP;
			main_surface.surface = s_raw_surface;
			GxAVSetProperty(s_device_handle, s_vpu_handle,
							GxVpuPropertyID_LayerMainSurface, (void*)&main_surface, sizeof(GxVpuProperty_LayerMainSurface));
		}

		if (s_surface_en == 1) {
			enable.layer  = GX_LAYER_SPP;
			enable.enable = 1;
			GxAVSetProperty(s_device_handle, s_vpu_handle,
							GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
		}

		if (s_virwidth && s_virheight) {
			ViewPort.layer = GX_LAYER_SPP;
			ViewPort.rect.x = 0;
			ViewPort.rect.y = 0;
			ViewPort.rect.width= s_virwidth;
			ViewPort.rect.height= s_virheight;
			GxAVSetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerViewport, &ViewPort, sizeof(GxVpuProperty_LayerViewport));
		}
	}
}

static int init_spu(void* data)
{
	GxVpuProperty_CreateSurface CreateSurface;
	GxVpuProperty_LayerMainSurface MainSurface;
	GxVpuProperty_LayerEnable SetLayerEnable;
	GxVpuProperty_LayerOnTop  SetLayerTop;
	GxVpuProperty_Resolution  VirtResolution;
	GxVpuProperty_LayerViewport ViewPort;
	PlayerWindow* rect = (PlayerWindow*)data;

	if(rect == NULL)
		return -1;

	s_device_handle = GxAvdev_CreateDevice(0);
	s_vpu_handle    = GxAvdev_OpenModule(s_device_handle, GXAV_MOD_VPU, 0);

	save_spu(0);
	hide_spu();

	GxAVGetProperty(s_device_handle,s_vpu_handle,
			GxVpuPropertyID_VirtualResolution, &VirtResolution, sizeof(GxVpuProperty_Resolution));

	if ((rect->width == 0)||( rect->height == 0)) {
		rect->width = 720;
		rect->height = 576;
	}

	s_spu_width  = rect->width;
	s_spu_height = rect->height;
	s_width  = (VirtResolution.xres > rect->width)?VirtResolution.xres:rect->width;
	s_height = (VirtResolution.yres > rect->height)?VirtResolution.yres:rect->height;

	SetLayerEnable.layer  = GX_LAYER_SPP;
	SetLayerEnable.enable = 0;
	GxAVSetProperty(s_device_handle,s_vpu_handle,
			GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

	SetLayerTop.layer  = GX_LAYER_SPP;
	SetLayerTop.enable = 1;
	GxAVSetProperty(s_device_handle, s_vpu_handle,
			GxVpuPropertyID_LayerOnTop, (void*)&SetLayerTop, sizeof(GxVpuProperty_LayerOnTop));

	CreateSurface.width  = s_width;
	CreateSurface.height = s_height;
	CreateSurface.format = GX_COLOR_FMT_YCBCRA6442;
	CreateSurface.mode   = GX_SURFACE_MODE_IMAGE;
	CreateSurface.buffer = NULL;
	GxAVGetProperty(s_device_handle, s_vpu_handle,
			GxVpuPropertyID_CreateSurface, (void*)&CreateSurface, sizeof(GxVpuProperty_CreateSurface));

	MainSurface.layer   = GX_LAYER_SPP;
	MainSurface.surface = CreateSurface.surface;
	GxAVSetProperty(s_device_handle,s_vpu_handle,
			GxVpuPropertyID_LayerMainSurface, &MainSurface, sizeof(GxVpuProperty_LayerMainSurface));

	s_new_surface = CreateSurface.surface;
#ifdef LINUX_OS
	s_new_buffer = (uint16_t*)mmap(0, s_width*s_height*2, PROT_READ | PROT_WRITE, MAP_SHARED, s_device_handle, (uint32_t)CreateSurface.buffer);
#else
	s_new_buffer = (uint16_t*)CreateSurface.buffer;
#endif

	clean_spu();

	ViewPort.layer = GX_LAYER_SPP;
	ViewPort.rect.x = 0;
	ViewPort.rect.y = 0;
	ViewPort.rect.width	= VirtResolution.xres;
	ViewPort.rect.height= VirtResolution.yres;
	GxAVSetProperty(s_device_handle,s_vpu_handle,
			GxVpuPropertyID_LayerViewport, &ViewPort, sizeof(GxVpuProperty_LayerViewport));

	SetLayerEnable.layer = MainSurface.layer;
	SetLayerEnable.enable = 1;
	GxAVSetProperty(s_device_handle,s_vpu_handle,
			GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

	return 0;
}

static inline void destroy_spu(void)
{
	GxVpuProperty_DestroySurface DestroySurface;

	hide_spu();
	save_spu(1);

	DestroySurface.surface = s_new_surface;
	GxAVSetProperty(s_device_handle, s_vpu_handle,
			GxVpuPropertyID_DestroySurface, (void*)&DestroySurface, sizeof(GxVpuProperty_DestroySurface));

	s_new_buffer  = NULL;
	s_new_surface = NULL;

	GxAvdev_CloseModule(s_device_handle, s_vpu_handle);
	GxAvdev_DestroyDevice(s_device_handle);
}


static void draw_spu(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride,void* data)
{
	int x, y,i;
	GxSpuDec* spu = (GxSpuDec*)data;
	uint32_t start_col = 0, start_row = 0;

	if (s_spu_width < s_width) {
		start_col = spu->start_col + (s_width - s_spu_width)/2;
	}

	if (s_spu_height < s_height) {
		if(spu->start_row > (s_spu_height/2))
			start_row = (s_height - s_spu_height)/2 +spu->start_row;
		else
			start_row = spu->start_row;
	}

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			int res;
			if (src[x] && (int)srca[x]) {
				if (spu->custom) {
					for (i=0;i<4;i++) {
						if (src[x]==(spu->cuspal[i]>>16)) {
							res = spu->cuspal[i];
							point_spu(start_col+x,start_row+y,res);
							break;
						}
					}
				} else {
					for (i=0;i<16;i++) {
						if (src[x]==(spu->global_palette[i]>>16)) {
							res = spu->global_palette[i];
							point_spu(start_col+x,start_row+y,res);
							break;
						}
					}
				}
			}
		}
		src  += spu->stride;
		srca += spu->stride;
	}
}

GxSubRender sub_render_yuv = {
	.init  = init_spu,
	.draw  = draw_spu,
	.hide  = hide_spu,
	.show  = show_spu,
	.clean = clean_spu,
	.destroy = destroy_spu,
};



#endif
