#include "gxcore.h"
#include "assert.h"
#include "gxavdev.h"
#include "gxaribcc.h"
#include "aribcc.h"
#include "module/app_log.h"

#if ARIBCC_SUPPORT
struct gxaribcc_render {
	int32_t                 dev;
	handle_t                osd;
	void*                   surface;
	GxAribCC_RenderRect   rect;
	GxAribCC_RenderCB cb;
	uint32_t*               buf;
};

static int32_t aribcc_render_clear(handle_t handle,int x, int y,int w,int h)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
    if(render == NULL)
        app_log_error("render is NULL!!!!!!!\n");

	ASSERT(render != NULL);

	return E_OK;
}

static handle_t aribcc_render_open(GxAribCC_RenderRect* rect,void *surface,void *buffer)
{
	struct gxaribcc_render*               render;

	render = CALLOC(1, sizeof(struct gxaribcc_render));
	if (render == NULL) {
		return E_INVALID_HANDLE;
	}

	render->rect.x      = rect->x;
	render->rect.y      = rect->y;
	render->rect.width  = rect->width;
	render->rect.heigth = rect->heigth;

	return (handle_t)render;
}

static int32_t aribcc_render_cb(handle_t handle,GxAribCC_RenderCB cb)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;

    if(render == NULL)
        app_log_error("render is NULL!!!!!!!\n");
	ASSERT(render != NULL);

	render->cb.draw_data = cb.draw_data;
	render->cb.clean_data = cb.clean_data ;

	return E_OK;
}

static int32_t aribcc_render_close(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;

	ASSERT(render != NULL);

	FREE(render);

	return E_OK;
}

static int32_t aribcc_render_show(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
    if(render == NULL)
        app_log_error("render is NULL!!!!!!!\n");

	ASSERT(render != NULL);

	return E_OK;
}

static int32_t aribcc_render_hide(handle_t handle)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;
    if(render == NULL)
        app_log_error("render is NULL!!!!!!!\n");

	ASSERT(render != NULL);

	return E_OK;
}

static int32_t aribcc_render_fill_region(handle_t handle, int8_t *data, uint32_t len)
{
	struct gxaribcc_render* render = (struct gxaribcc_render*)handle;

	ASSERT(render != NULL);

	if(data && render->cb.draw_data)
		render->cb.draw_data(arib_get_cur_lang(),NULL,data,len);
	else if(render->cb.clean_data)
		render->cb.clean_data(NULL);

	return E_OK;
}

GxAribCC_RenderOps aribcc_render_osd = {
	.open           = aribcc_render_open,
	.registercb     = aribcc_render_cb,
	.close          = aribcc_render_close,
	.clear          = aribcc_render_clear,
	.show           = aribcc_render_show,
	.hide           = aribcc_render_hide,
	.fill_region    = aribcc_render_fill_region,
};
#endif

