#include "gxcore.h"
#include "gui_core.h"
#include "gxavdev.h"
struct rect{
	unsigned int x,y,w,h;
};

struct vpu{
	int dev;
	int vpu;
	void *spp_surface;
	void *osd_surface;
};


struct vpu myvpu;

#define COLOR_KEY     (0xff)
#define CAP_WIDTH     (1920)
#define CAP_HEIGHT    (1080/4 - 1)


static int fill(void *surface, struct rect * dstrect, unsigned int color)
{

	int ret;
	GxVpuProperty_FillRect FillRect = {0};
	if (surface == myvpu.osd_surface){
		FillRect.color.r = (color & 0xF800) >> 8;
		FillRect.color.g = (color & 0x07E0) >> 3;
		FillRect.color.b = (color & 0x001F) << 3;
	}
	if (surface == myvpu.spp_surface){
		FillRect.color.y = (color >> 10) & 0x0000003F;
		FillRect.color.cb = (color >> 6) & 0x0000000F;
		FillRect.color.cr = (color>>2)   & 0x0000000F;
		FillRect.color.alpha = color	 & 0x00000003;
	}

	FillRect.rect.x = dstrect->x;
	FillRect.rect.y = dstrect->y;
	FillRect.rect.width = dstrect->w;
	FillRect.rect.height = dstrect->h;
	FillRect.is_ksurface = 1;
	FillRect.surface = surface;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));

	if(0 != ret)
		printf("[bbt]Filled Rectangle failed!\n");
	return ret;
}

static int copy(void *srcsurface,void *dstsurface, struct rect * srcrect, struct rect * dstrect)
{
	GxVpuProperty_Blit  Blit;
	int ret = 0;

	/*copy captured vpp data thrice to fill full screen*/
	memset(&Blit,0,sizeof(GxVpuProperty_Blit));
	Blit.srca.is_ksurface = 1;
	Blit.srca.surface       = srcsurface;
	Blit.srca.rect.x        = srcrect->x;
	Blit.srca.rect.y        = srcrect->y;
	Blit.srca.rect.width    = srcrect->w;
	Blit.srca.rect.height   = srcrect->h;

	Blit.srcb.is_ksurface = 1;
	Blit.srcb.surface       = dstsurface;
	Blit.srcb.rect.x        = dstrect->x;
	Blit.srcb.rect.y        = dstrect->y;
	Blit.srcb.rect.width    = dstrect->w;
	Blit.srcb.rect.height   = dstrect->h;
	Blit.dst = Blit.srcb;

	Blit.mode = GX_ALU_ROP_COPY;
	ret = GxAVSetProperty(myvpu.dev,myvpu.vpu,GxVpuPropertyID_Blit,&Blit,sizeof(Blit));
	return ret;

}
int enable_layer(int layer,int on)
{
	int ret = 0;
	GxVpuProperty_LayerEnable set_layer_enable;
	set_layer_enable.layer = layer;
	set_layer_enable.enable = on;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerEnable, &set_layer_enable, sizeof(GxVpuProperty_LayerEnable));
	return ret;
}

static int init_spp_layer(int x,int y)
{
	int ret = 0;
	GxVpuProperty_CreateSurface spp_surface;
	GxVpuProperty_LayerMainSurface main_surface;
	GxVpuProperty_Alpha alpha;

	spp_surface.width = x;
	spp_surface.height = y;
	spp_surface.mode = GX_SURFACE_MODE_IMAGE;
	spp_surface.format = GX_COLOR_FMT_YCBCRA6442;

	spp_surface.buffer = NULL;

	ret = GxAVGetProperty(myvpu.dev, myvpu.vpu, GxVpuPropertyID_CreateSurface,
			&spp_surface, sizeof(GxVpuProperty_CreateSurface));
	if(0 != ret)
		printf("[bbt]Create SPP surface failed!\n");

	main_surface.layer = GX_LAYER_SPP;
	main_surface.surface = spp_surface.surface;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerMainSurface, &main_surface, sizeof(GxVpuProperty_LayerMainSurface));
	if(0 != ret)
		printf("[bbt]Set SPP main surface failed!\n");

	/* config spp zoom */
	GxVpuProperty_LayerViewport Spp_Viewport;
	Spp_Viewport.layer = GX_LAYER_SPP;
	Spp_Viewport.rect.x = 0;
	Spp_Viewport.rect.y = 0;
	Spp_Viewport.rect.width = x;
	Spp_Viewport.rect.height = y;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerViewport,
			&Spp_Viewport,
			sizeof(GxVpuProperty_LayerViewport));
	if(0 != ret)
		printf("line(%d) GxVpuPropertyID_LayerViewport Error! ret=%d\n", __LINE__, ret);

	alpha.surface = spp_surface.surface;
	alpha.alpha.type = GX_ALPHA_GLOBAL;
	alpha.alpha.value = 0;
	ret = GxAVSetProperty(myvpu.dev, myvpu.vpu, GxVpuPropertyID_Alpha, &alpha, sizeof(GxVpuProperty_Alpha));
	if(0 != ret)
		printf("[bbt]SPP surface set alpha failed!\n");

	myvpu.spp_surface = spp_surface.surface;
	struct rect rect = {0,0,x,y};
	fill(myvpu.spp_surface, &rect, 0xfff0);
	enable_layer(GX_LAYER_SPP,0);

	return 0;
}

static int init_osd_layer(int x,int y)
{
	int ret = 0;
	GxVpuProperty_CreateSurface osd_surface;
	GxVpuProperty_LayerOnTop  Set_LayeOnTop;
	GxVpuProperty_LayerMainSurface main_surface;
	GxVpuProperty_Alpha alpha;
	GxVpuProperty_ColorKey color_key = { 0 };

	osd_surface.width = x;
	osd_surface.height = y;
	osd_surface.mode = GX_SURFACE_MODE_IMAGE;
	osd_surface.buffer = NULL;
	osd_surface.format = GX_COLOR_FMT_RGB565;

	ret = GxAVGetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_CreateSurface, &osd_surface, sizeof(GxVpuProperty_CreateSurface));

	if(0 != ret)
		printf("[bbt]Create OSD surface failed!\n");

	//memset(osd_surface.buffer, 0xff, x * y * 2);

	alpha.surface = osd_surface.surface;
	alpha.alpha.type = GX_ALPHA_GLOBAL;
	alpha.alpha.value = 0x7F;

	ret = GxAVSetProperty(myvpu.dev, myvpu.vpu, GxVpuPropertyID_Alpha, &alpha, sizeof(GxVpuProperty_Alpha));
	if(0 != ret)
		printf("[bbt]OSD surface set alpha failed!\n");

	color_key.enable = 1;
	color_key.color.r = (COLOR_KEY & 0xF800) >> 8;
	color_key.color.g = (COLOR_KEY & 0x07E0) >> 3;
	color_key.color.b = (COLOR_KEY & 0x001F) << 3;
	color_key.color.a = 0;
	color_key.surface = osd_surface.surface;

	ret = GxAVSetProperty(myvpu.dev, myvpu.vpu,
			GxVpuPropertyID_ColorKey, &color_key, sizeof(GxVpuProperty_ColorKey));
	if(0 != ret)
		printf("[bbt]Set OSD Color Key failed!\n");

	main_surface.layer = GX_LAYER_OSD;
	main_surface.surface = osd_surface.surface;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerMainSurface, &main_surface, sizeof(GxVpuProperty_LayerMainSurface));
	if(0 != ret)
		printf("[bbt]Set OSD Main surface failed!\n");

	/* config osd zoom */
	GxVpuProperty_LayerViewport Osd_Viewport;
	Osd_Viewport.layer = GX_LAYER_OSD;
	Osd_Viewport.rect.x = 0;
	Osd_Viewport.rect.y = 0;
	Osd_Viewport.rect.width = x;
	Osd_Viewport.rect.height = y;
	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerViewport,
			&Osd_Viewport,
			sizeof(GxVpuProperty_LayerViewport));
	if(0 != ret)
	{
		printf("line(%d) GxVpuPropertyID_LayerViewport Error! ret=%d\n", __LINE__, ret);
	}


	Set_LayeOnTop.layer = GX_LAYER_SPP;
	//Set_LayeOnTop.layer = GX_LAYER_OSD;
	Set_LayeOnTop.enable = 1;

	ret = GxAVSetProperty(myvpu.dev,
			myvpu.vpu,
			GxVpuPropertyID_LayerOnTop,
			&Set_LayeOnTop,
			sizeof(GxVpuProperty_LayerOnTop));
	if(0 != ret)
		printf("[bbt]Set SPP top enable failed!\n");

	myvpu.osd_surface = osd_surface.surface;
	struct rect rect = {0,0,x,y};
	fill(myvpu.osd_surface, &rect, 0xff);
	enable_layer(GX_LAYER_OSD,0);
	//enable_layer(GX_LAYER_OSD,1);
	return 0;
}

void vpu_init(int x,int y)
{
	myvpu.dev = GxAvdev_CreateDevice(0);
	if (myvpu.dev <= 0)
		printf("[bbt]:Cannot open device, %d!\n",myvpu.dev);

	myvpu.vpu = GxAvdev_OpenModule(myvpu.dev, GXAV_MOD_VPU, 0);
	if (myvpu.vpu <= 0)
		printf("[bbt]Open VPU module failed!\n");

	init_spp_layer(x,y);
	init_osd_layer(x,y);
}



