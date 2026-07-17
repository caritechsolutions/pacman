#include "gxcore.h"
#include "assert.h"
#include "gxavdev.h"
#include "gxaribcc.h"
#include "aribcc.h"
#include "module/app_log.h"

#ifdef LINUX_OS
#include <sys/mman.h>
#endif

#if ARIBCC_SUPPORT
struct gxaribcc_render {
	int32_t                 dev;
	handle_t             spp;
	handle_t		 mutex;
	void*                   surface;
	int32_t			spp_self;
	GxAribCC_RenderRect   rect;
	GxAribCC_RenderClut   clut;
	int				old_color_format;
	void*                   old_surface;
	int                    old_top;
	int                    old_enable;
	GxAribCC_RenderRect   old_rect;
	uint16_t*               buf;
	GxAribCC_RenderCB cb;
};

static int32_t aribcc_render_clear(handle_t handle,int x, int y,int w,int h)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
	uint16_t *buf = (uint16_t*)render->buf;
	int i = 0,j = 0;

	GxCore_MutexLock(render->mutex);

	for (i=0; i < h; ++i)
		for (j=0; j < w; ++j)
			*(buf+y*render->rect.width+x+render->rect.width*i+j) = 0;

	GxCore_MutexUnlock(render->mutex);
	return E_OK;
}

static handle_t aribcc_render_open(GxAribCC_RenderRect* rect,void *surface,void *buffer)
{
	struct gxaribcc_render*               render;
	GxVpuProperty_ColorFormat       format = {0};
	GxVpuProperty_CreateSurface     p = {0};
	GxVpuProperty_LayerMainSurface  layer = {0};
	GxVpuProperty_LayerEnable       enable = {0};
	GxVpuProperty_LayerOnTop        top = {0};
	GxVpuProperty_ColorFormat		colorformat = {0};
	GxVpuProperty_LayerViewport     viewport = {0};
	int32_t                         ret;

	render = CALLOC(1, sizeof(struct gxaribcc_render));
	if (render == NULL) {
		return E_INVALID_HANDLE;
	}

	GxCore_MutexCreate(&render->mutex);

	render->dev    = GxAvdev_CreateDevice(0);
	render->spp    = GxAvdev_OpenModule(render->dev, GXAV_MOD_VPU, 0);

	GxCore_MutexLock(render->mutex);

	render->rect.x      = rect->x;
	render->rect.y      = rect->y;
	render->rect.width  = rect->width;
	render->rect.heigth = rect->heigth;

	if(buffer != NULL)
	{
		GxVpuProperty_ReadSurface readsurface;
		GxVpuProperty_LayerViewport     viewport;
		GxVpuProperty_VirtualResolution resolution;

		ASSERT(surface);

		render->surface = surface;
		render->buf = (uint16_t *)buffer;

		render->spp_self = 0;

		memset(&readsurface,0,sizeof(GxVpuProperty_ReadSurface));

		readsurface.surface = surface;
		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_ReadSurface,
				&readsurface,
				sizeof(GxVpuProperty_ReadSurface));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuPropertyID_ReadSurface failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		if(readsurface.format != GX_COLOR_FMT_YCBCR422 &&
				readsurface.format != GX_COLOR_FMT_YCBCRA6442)
		{
			GxAvdev_CloseModule(render->dev, render->spp);
			GxAvdev_DestroyDevice(render->dev);
			FREE(render);
			return 0;
		}

		//get
		render->old_color_format = readsurface.format;

		layer.layer = GX_LAYER_SPP;
		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerMainSurface,
				&layer,
				sizeof(GxVpuProperty_LayerMainSurface));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuPropertyID_LayerMainSurface failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);
		render->old_surface = layer.surface;

		top.layer  = GX_LAYER_SPP;
		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerOnTop,
				&top,
				sizeof(GxVpuProperty_LayerOnTop));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuPropertyID_LayerOnTop failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);
		render->old_top = top.enable;

		enable.layer  = GX_LAYER_SPP;
		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerEnable,
				&enable,
				sizeof(GxVpuProperty_LayerEnable));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuPropertyID_LayerEnable failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);
		render->old_enable = enable.enable;

		//set
		layer.layer      = GX_LAYER_SPP;
		layer.surface    = render->surface;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerMainSurface,
				&layer,
				sizeof(GxVpuProperty_LayerMainSurface));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerMainSurface failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		colorformat.surface  = render->surface;
		colorformat.format = GX_COLOR_FMT_YCBCRA6442;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_ColorFormat, (void*)&colorformat,
				sizeof(GxVpuProperty_ColorFormat));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_ColorFormat failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_VirtualResolution,
				(void*)&resolution,
				sizeof(GxVpuProperty_VirtualResolution));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_VirtualResolution failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		viewport.layer = GX_LAYER_SPP;
		viewport.rect.x = 0;
		viewport.rect.y = 0;
		viewport.rect.width = resolution.xres;
		viewport.rect.height = resolution.yres;

		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerViewport,
				&viewport,
				sizeof(GxVpuProperty_LayerViewport));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerViewport failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		GxCore_MutexUnlock(render->mutex);

		aribcc_render_clear((handle_t)render,0,0,render->rect.width,render->rect.heigth);

		return (handle_t)render;
	}

	render->spp_self = 1;

	top.layer  = GX_LAYER_SPP;
	ret = GxAVGetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerOnTop,
			&top,
			sizeof(GxVpuProperty_LayerOnTop));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuPropertyID_LayerOnTop failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);
	render->old_top = top.enable;

	enable.layer  = GX_LAYER_SPP;
	ret = GxAVGetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerEnable,
			&enable,
			sizeof(GxVpuProperty_LayerEnable));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuPropertyID_LayerEnable failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);
	render->old_enable = enable.enable;

	p.surface   = NULL,
	p.width     = rect->width,
	p.height    = rect->heigth,
	p.format    = GX_COLOR_FMT_YCBCRA6442;
	p.mode      = GX_SURFACE_MODE_IMAGE,
	p.buffer    = NULL;
	ret = GxAVGetProperty(render->dev, render->spp,
			GxVpuPropertyID_CreateSurface,
			&p,
			sizeof(GxVpuProperty_CreateSurface));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuPropertyID_CreateSurface failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

#ifdef LINUX_OS
	render->buf = (uint16_t*)mmap(0,
			rect->width*rect->heigth*2,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			render->dev,
			(uint32_t)p.buffer);
#else
	render->buf = p.buffer;
#endif
	render->surface = p.surface;

	format.surface  = p.surface;
	format.format   = GX_COLOR_FMT_YCBCRA6442;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_ColorFormat,
			&format,
			sizeof(GxVpuProperty_ColorFormat));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_ColorFormat failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	viewport.layer = GX_LAYER_SPP;
	ret = GxAVGetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerViewport,
			&viewport,
			sizeof(GxVpuProperty_LayerViewport));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerViewport failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);
	render->old_rect.x = viewport.rect.x;
	render->old_rect.y = viewport.rect.y;
	render->old_rect.width = viewport.rect.width;
	render->old_rect.heigth = viewport.rect.height;

	GxCore_MutexUnlock(render->mutex);

	aribcc_render_clear((handle_t)render,0,0,render->rect.width,render->rect.heigth);

	return (handle_t)render;
}

static int32_t aribcc_render_cb(handle_t handle,GxAribCC_RenderCB cb)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;

	ASSERT(render != NULL);

	render->cb.draw_data = cb.draw_data;
	render->cb.clean_data = cb.clean_data ;

	return E_OK;
}

static int32_t aribcc_render_close(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
	GxVpuProperty_DestroySurface    p = {0};
	GxVpuProperty_LayerMainSurface  layer = {0};
	GxVpuProperty_LayerEnable       enable = {0};
	GxVpuProperty_LayerOnTop        top = {0};
	GxVpuProperty_ColorFormat		colorformat = {0};
#ifdef LINUX_OS
	GxVpuProperty_ReadSurface		readsurface = {0};
#endif
	int32_t                         ret;

	ASSERT(render != NULL);

	GxCore_MutexLock(render->mutex);

	if(render->spp_self == 0)
	{
		GxVpuProperty_LayerViewport     viewport;
		GxVpuProperty_VirtualResolution resolution;

		if(render->old_surface)
		{
			layer.layer      = GX_LAYER_SPP;
			layer.surface    = render->old_surface;
			ret = GxAVSetProperty(render->dev, render->spp,
					GxVpuPropertyID_LayerMainSurface,
					&layer,
					sizeof(GxVpuProperty_LayerMainSurface));
            if(ret != GXCORE_SUCCESS)
                app_log_error("aribcc GxVpuPropertyID_LayerMainSurface failed!!!!!!!\n");
			ASSERT(ret == GXCORE_SUCCESS);

			colorformat.surface  = render->old_surface;
			colorformat.format = render->old_color_format;
			ret = GxAVSetProperty(render->dev, render->spp,
					GxVpuPropertyID_ColorFormat, (void*)&colorformat,
					sizeof(GxVpuProperty_ColorFormat));
            if(ret != GXCORE_SUCCESS)
                app_log_error("aribcc GxVpuPropertyID_ColorFormat failed!!!!!!!\n");
			ASSERT(ret == GXCORE_SUCCESS);
		}

		top.layer  = GX_LAYER_SPP;
		top.enable = render->old_top;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerOnTop, (void*)&top,
				sizeof(GxVpuProperty_LayerOnTop));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerOnTop failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		enable.enable       = render->old_enable;
		enable.layer         = GX_LAYER_SPP;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerEnable,
				&enable,
				sizeof(GxVpuProperty_LayerEnable));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerEnable failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		ret = GxAVGetProperty(render->dev, render->spp,
				GxVpuPropertyID_VirtualResolution,
				(void*)&resolution,
				sizeof(GxVpuProperty_VirtualResolution));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_VirtualResolution failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		viewport.layer = GX_LAYER_SPP;
		viewport.rect.x = 0;
		viewport.rect.y = 0;
		viewport.rect.width = resolution.xres;
		viewport.rect.height = resolution.yres;

		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerViewport,
				&viewport,
				sizeof(GxVpuProperty_LayerViewport));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerViewport failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		GxCore_MutexUnlock(render->mutex);
		GxCore_MutexDelete(render->mutex);

		GxAvdev_CloseModule(render->dev, render->spp);
		GxAvdev_DestroyDevice(render->dev);

		FREE(render);

		return E_OK;
	}

#ifdef LINUX_OS
	readsurface.surface = render->surface;
	ret = GxAVGetProperty(render->dev,
			render->spp,
			GxVpuPropertyID_ReadSurface,
			&readsurface,
			sizeof(GxVpuProperty_ReadSurface));
	if(0 == ret)
		munmap(render->buf, readsurface.width * readsurface.height * 2);
#endif

	p.surface = render->surface;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_DestroySurface,
			&p,
			sizeof(GxVpuProperty_DestroySurface));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_DestroySurface failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	top.layer  = GX_LAYER_SPP;
	top.enable = render->old_top;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerOnTop, (void*)&top,
			sizeof(GxVpuProperty_LayerOnTop));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuPropertyID_LayerOnTop failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	GxCore_MutexUnlock(render->mutex);
	GxCore_MutexDelete(render->mutex);

	GxAvdev_CloseModule(render->dev, render->spp);
	GxAvdev_DestroyDevice(render->dev);

	FREE(render);

	return E_OK;
}

static int32_t aribcc_render_show(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
	GxVpuProperty_LayerMainSurface  layer = {0};
	GxVpuProperty_LayerEnable       p = {0};
	GxVpuProperty_LayerOnTop        top = {0};
	GxVpuProperty_LayerViewport     viewport = {0};
	GxVpuProperty_VirtualResolution resolution = {0};
	int32_t                         ret;

	ASSERT(render != NULL);

	if(render->spp_self == 0)
	{
		GxCore_MutexLock(render->mutex);

		top.layer  = GX_LAYER_SPP;
		top.enable = 1;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerOnTop,
				(void*)&top,
				sizeof(GxVpuProperty_LayerOnTop));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerOnTop failed!!!!!!!\n");
		ASSERT(ret == GXCORE_SUCCESS);

		p.enable       = 1;
		p.layer         = GX_LAYER_SPP;
		ret = GxAVSetProperty(render->dev, render->spp,
				GxVpuPropertyID_LayerEnable,
				&p,
				sizeof(GxVpuProperty_LayerEnable));
        if(ret != GXCORE_SUCCESS)
            app_log_error("aribcc GxVpuProperty_LayerEnable failed!!!!!!!\n");

		ASSERT(ret == GXCORE_SUCCESS);

		GxCore_MutexUnlock(render->mutex);

		return 0;
	}

	GxCore_MutexLock(render->mutex);

	layer.layer      = GX_LAYER_SPP;
	layer.surface    = render->surface;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerMainSurface,
			&layer,
			sizeof(GxVpuProperty_LayerMainSurface));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerMainSurface failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	ret = GxAVGetProperty(render->dev, render->spp,
			GxVpuPropertyID_VirtualResolution,
			(void*)&resolution,
			sizeof(GxVpuProperty_VirtualResolution));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_VirtualResolution failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	viewport.layer = GX_LAYER_SPP;
	viewport.rect.x = 0;
	viewport.rect.y = 0;
	viewport.rect.width = resolution.xres;
	viewport.rect.height = resolution.yres;

	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerViewport,
			&viewport,
			sizeof(GxVpuProperty_LayerViewport));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerViewport failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	top.layer  = GX_LAYER_SPP;
	top.enable = 1;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerOnTop,
			(void*)&top,
			sizeof(GxVpuProperty_LayerOnTop));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerOnTop failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	p.enable        = 1;
	p.layer         = GX_LAYER_SPP;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerEnable,
			&p,
			sizeof(GxVpuProperty_LayerEnable));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerEnable failed!!!!!!!\n");
    ASSERT(ret == GXCORE_SUCCESS);

	GxCore_MutexUnlock(render->mutex);

	return E_OK;
}

static int32_t aribcc_render_hide(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
	GxVpuProperty_LayerEnable       p = {0};
	int32_t                         ret;

	ASSERT(render != NULL);

	GxCore_MutexLock(render->mutex);

	p.enable        = 0;
	p.layer         = GX_LAYER_SPP;
	ret = GxAVSetProperty(render->dev, render->spp,
			GxVpuPropertyID_LayerEnable,
			&p,
			sizeof(GxVpuProperty_LayerEnable));
    if(ret != GXCORE_SUCCESS)
        app_log_error("aribcc GxVpuProperty_LayerEnable failed!!!!!!!\n");
	ASSERT(ret == GXCORE_SUCCESS);

	GxCore_MutexUnlock(render->mutex);

	return E_OK;
}

static int32_t aribcc_render_fill_region(handle_t handle, int8_t *data, uint32_t len)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
	GxAribCC_RenderSize size;

	ASSERT(render != NULL);

	size.width = render->rect.width;
	size.height = render->rect.heigth;
	size.surface = render->surface;
	size.buffer = render->buf;

	if(data && render->cb.draw_data)
		render->cb.draw_data(arib_get_cur_lang(),&size,data,len);
	else if(render->cb.clean_data)
		render->cb.clean_data(&size);

	return E_OK;
}


GxAribCC_RenderOps aribcc_render_spp = {
	.open           = aribcc_render_open,
	.registercb     = aribcc_render_cb,
	.close          = aribcc_render_close,
	.clear          = aribcc_render_clear,
	.show           = aribcc_render_show,
	.hide           = aribcc_render_hide,
	.fill_region    = aribcc_render_fill_region,
};
#endif

