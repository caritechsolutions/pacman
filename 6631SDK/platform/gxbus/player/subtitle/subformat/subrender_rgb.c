
#include "gx_subrender.h"
#include "spudec.h"
#ifdef LINUX_OS
#include <sys/mman.h>
#endif

#ifndef I386_LINUX


static int              s_device_handle;
static handle_t         s_vpu_handle;
static void*            s_new_surface;
static void*            s_raw_surface;
static char*            s_surface_buffer;
static GxPalette*       s_new_palette;

static int              tranidx;
static uint8_t          s_surface_en;
static uint8_t          s_antiflicker_en;
static uint8_t          s_global_alpha_en;
static uint8_t          s_global_alpha;

static void inline point_rgb(int x,int y,int index)
{
	#if 0
	GxVpuProperty_Point Point;
	Point.surface = s_new_surface;

	Point.color.entry = index;
	Point.point.x = x;
	Point.point.y = y;

	GxAVSetProperty(s_device_handle,
			      s_vpu_handle,
			      GxVpuPropertyID_Point, &Point, sizeof(GxVpuProperty_Point));
	#else
	s_surface_buffer[720*y+x]=index&0xff;
	#endif
}

static inline void save_rgb(int flag)
{
	GxVpuProperty_LayerMainSurface  main_surface;
	GxVpuProperty_LayerEnable       enable;
	GxVpuProperty_DestroySurface    DestroySurface;
	GxVpuProperty_DestroyPalette    DestroyPalette;
	GxVpuProperty_LayerAntiFlicker  AntiFlicker;
	GxVpuProperty_Alpha             Alpha;

	if(flag ==0)
	{
		main_surface.layer = GX_LAYER_OSD;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerMainSurface, (void*)&main_surface, sizeof(GxVpuProperty_LayerMainSurface));
		s_raw_surface = main_surface.surface;

		enable.layer  = GX_LAYER_OSD;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
		s_surface_en = enable.enable;

		AntiFlicker.layer  = GX_LAYER_OSD;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerAntiFlicker, (void*)&AntiFlicker, sizeof(GxVpuProperty_LayerAntiFlicker));

		s_antiflicker_en = AntiFlicker.enable;

		Alpha.surface = s_raw_surface;
		GxAVGetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_Alpha, (void*)&Alpha, sizeof(GxVpuProperty_Alpha));

		s_global_alpha    = Alpha.alpha.type;
		s_global_alpha_en = Alpha.alpha.value;

	}
	else
	{
		enable.layer  = GX_LAYER_OSD;
		enable.enable = 0;
		GxAVSetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));

		if(s_raw_surface != NULL){
			main_surface.layer = GX_LAYER_OSD;
			main_surface.surface = s_raw_surface;
			GxAVSetProperty(s_device_handle, s_vpu_handle,
				GxVpuPropertyID_LayerMainSurface, (void*)&main_surface, sizeof(GxVpuProperty_LayerMainSurface));
		}

		if(s_antiflicker_en == 0){
			AntiFlicker.layer = GX_LAYER_OSD;
			AntiFlicker.enable = 0;
			GxAVSetProperty(s_device_handle,
					      s_vpu_handle,
					      GxVpuPropertyID_LayerAntiFlicker,
					      &AntiFlicker, sizeof(GxVpuProperty_LayerAntiFlicker));
		}

		if(s_global_alpha_en == GX_ALPHA_GLOBAL){
			Alpha.surface = s_raw_surface;
			Alpha.alpha.type = GX_ALPHA_GLOBAL;
			Alpha.alpha.value = s_global_alpha;
			GxAVSetProperty(s_device_handle, s_vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));
		}

		GxAVSetProperty(s_device_handle, s_vpu_handle,
				GxVpuPropertyID_Alpha, (void*)&s_global_alpha, sizeof(GxVpuProperty_Alpha));

		if(s_surface_en == 1){
			enable.layer  = GX_LAYER_OSD;
			enable.enable = 1;
			GxAVSetProperty(s_device_handle, s_vpu_handle,
					GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
		}

		DestroyPalette.palette = s_new_palette;
		GxAVSetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_DestroyPalette, (void*)&DestroyPalette, sizeof(GxVpuProperty_DestroyPalette));

		DestroySurface .surface = s_new_surface;
		GxAVSetProperty(s_device_handle, s_vpu_handle,
						GxVpuPropertyID_DestroySurface, (void*)&DestroySurface, sizeof(GxVpuProperty_DestroySurface));
	}
}

static inline void clean_rgb(void)
{
	/*GxVpuProperty_FillRect FillRect;

	FillRect.rect.x = 0;
	FillRect.rect.y = 0;
	FillRect.rect.width = 720;
	FillRect.rect.height = 576;
	FillRect.surface = s_new_surface;
	FillRect.color.entry = tranidx;
	GxAVSetProperty(s_device_handle,
			      		       s_vpu_handle,
			      		       GxVpuPropertyID_FillRect,
			      		       &FillRect,
			      		       sizeof(GxVpuProperty_FillRect));*/

	memset(s_surface_buffer,tranidx,720*576);
}

static inline void hide_rgb(void)
{
	GxVpuProperty_LayerEnable SetLayerEnable;

	SetLayerEnable.layer = GX_LAYER_OSD;
	SetLayerEnable.enable = 0;
	GxAVSetProperty(s_device_handle,
	      s_vpu_handle,
	      GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));
}

static inline void show_rgb(void)
{
	GxVpuProperty_LayerEnable SetLayerEnable;

	SetLayerEnable.layer = GX_LAYER_OSD;
	SetLayerEnable.enable = 1;
	GxAVSetProperty(s_device_handle,
	      s_vpu_handle,
	      GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));
}

static int yuv_to_rgb(unsigned int yuv)
{
    int r, g, b, y, u, v;

    y = yuv >> 16 & 0xff;
    u = yuv >> 8 & 0xff;
    v = yuv & 0xff;
    r = (int)(y+1.4075*(v-128))&0xff;
    g = (int)(y-0.3455*(u-128)-0.7169*(v-128))&0xff;
    b = (int)(y+1.779*(u-128))&0xff;
    return r << 16 | g << 8 | b;
}

static void draw_rgb(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride,void* data)
{
    int x, y,i;
	GxSpuDec* spu = (GxSpuDec*)data;

	clean_rgb();
    for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
		    int res;
		    if (src[x] && (int)srca[x])
		    {
		    	if(spu->custom){
					for(i=0;i<4;i++){
						if(src[x]==(spu->cuspal[i]>>16)){
							res = i;
							point_rgb(x0+x,y0+y,res);
							break;
						}
					}
				}
				else{
					for(i=0;i<16;i++){
						if(src[x]==(spu->global_palette[i]>>16)){
							res = i;
							point_rgb(x0+x,y0+y,res);
							break;
						}
					}
				}
			}
		}
		src += stride;
		srca += stride;
    }
}


static inline void destroy_rgb(void)
{
	hide_rgb();
	save_rgb(1);
}

int init_rgb(void* data)
{
	int i,rgb,ret;
	GxPalette  palette = {0};
	GxColor entries[256] ;
	GxVpuProperty_RWPalette RWPalette = {0};

	GxVpuProperty_CreateSurface CreateSurface;
	GxVpuProperty_CreatePalette CreatePalette = {0};
	GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};

	GxVpuProperty_LayerAntiFlicker AntiFlicker;
	GxVpuProperty_LayerMainSurface MainSurface;
	GxVpuProperty_Alpha Alpha;

	GxSpuDec* spu = (GxSpuDec*)data;

	s_device_handle = GxAvdev_CreateDevice(0);

	s_vpu_handle = GxAvdev_OpenModule(s_device_handle, GXAV_MOD_VPU, 0);

	save_rgb(0);
	hide_rgb();

	CreateSurface.width = 720;
	CreateSurface.height = 576;
	CreateSurface.format = GX_COLOR_FMT_CLUT8;
	CreateSurface.mode = GX_SURFACE_MODE_IMAGE;
	CreateSurface.buffer = NULL;

	ret = GxAVGetProperty(s_device_handle, s_vpu_handle,
			      GxVpuPropertyID_CreateSurface, &CreateSurface, sizeof(GxVpuProperty_CreateSurface));

	if(ret != GXCORE_SUCCESS)
		goto error;

	s_new_surface = CreateSurface.surface;

#ifdef LINUX_OS
	s_surface_buffer = mmap(0,
			720*576,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			s_device_handle,
			(uint32_t)CreateSurface.buffer);
#else
	s_surface_buffer = (char*)CreateSurface.buffer;
#endif

	MainSurface.layer = GX_LAYER_OSD;
	MainSurface.surface =  CreateSurface.surface;
	ret = GxAVSetProperty(s_device_handle,
			      s_vpu_handle,
			      GxVpuPropertyID_LayerMainSurface, &MainSurface, sizeof(GxVpuProperty_LayerMainSurface));

	if(ret != GXCORE_SUCCESS)
		goto error;

	AntiFlicker.layer = GX_LAYER_OSD;
	AntiFlicker.enable = 1;
	ret = GxAVSetProperty(s_device_handle,
			      s_vpu_handle,
			      GxVpuPropertyID_LayerAntiFlicker,
			      &AntiFlicker, sizeof(GxVpuProperty_LayerAntiFlicker));

	Alpha.surface = CreateSurface.surface;
	Alpha.alpha.type = GX_ALPHA_PIXEL;
	Alpha.alpha.value = 127;

	ret = GxAVSetProperty(s_device_handle, s_vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));

	CreatePalette.num_entries = 256;
	CreatePalette.palette_num = 1;
	ret = GxAVGetProperty(s_device_handle,
						  s_vpu_handle,
						  GxVpuPropertyID_CreatePalette,
						  &CreatePalette,
						  sizeof(GxVpuProperty_CreatePalette));

	if(ret != GXCORE_SUCCESS)
		goto error;


	s_new_palette = CreatePalette.palette;

	SurfaceBindPalette.surface = s_new_surface;
	SurfaceBindPalette.palette = s_new_palette;
	ret = GxAVSetProperty(s_device_handle,
						  s_vpu_handle,
						  GxVpuPropertyID_SurfaceBindPalette,
						  &SurfaceBindPalette,
						  sizeof(GxVpuProperty_SurfaceBindPalette));

	if(ret != GXCORE_SUCCESS)
		goto error;

	if(spu->custom)
	{
		for(i=0;i<4;i++){
			rgb = yuv_to_rgb(spu->cuspal[i]);
			entries[i].r=rgb>> 16 & 0xff;
			entries[i].g=rgb>> 8  & 0xff;
			entries[i].b=rgb & 0xff;;
			entries[i].a=0xff;
			if(spu->alpha[i]==0)
				tranidx=i;
		}
	}
	else
	{
		for(i=0;i<16;i++){
			rgb = yuv_to_rgb(spu->global_palette[i]);
			entries[i].r=rgb>> 16 & 0xff;
			entries[i].g=rgb>> 8  & 0xff;
			entries[i].b=rgb & 0xff;;
			entries[i].a=0xff;
		}
		tranidx = 16;
	}

	RWPalette.k_palette = s_new_palette;
	RWPalette.palette_id = 0;
	palette.entries = entries;
	palette.num_entries = 256;
	RWPalette.u_palette = &palette;

	entries[tranidx].a = 0;

	ret = GxAVSetProperty(s_device_handle,
						  s_vpu_handle,
						  GxVpuPropertyID_RWPalette,
						  &RWPalette,
						  sizeof(GxVpuProperty_RWPalette));

	if(ret != GXCORE_SUCCESS)
		goto error;

	clean_rgb();

	show_rgb();

	return GX_PLAYER_OK;

error:

	destroy_rgb();

	return GX_PLAYER_ERROR;

}

GxSubRender sub_render_rgb = {
	.init  = init_rgb,
	.draw  = draw_rgb,
	.hide  = hide_rgb,
	.show  = show_rgb,
	.clean = clean_rgb,
	.destroy = destroy_rgb,
};

#endif


