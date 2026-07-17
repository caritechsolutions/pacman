#include "hd_gal.h"
#ifndef NO_OS
#include "gui_private.h"
int gdi_change_surface(void **src0, void **src1)
{
	return hd_change_surface(src0, src1);
}

int gxgdi_init(void)
{
	return (hd_init());
}

int gdi_lock(void)
{
	return (hd_lock());
}

int gdi_unlock(void)
{
	return (hd_unlock());
}

static void _set_hardlayer(GxLayerID layer_id, GuiCore *gui_core) {
	GuiSurface *surface = NULL;
	GXGDI_Rect rect = {0, 0, gui_core->config.width, gui_core->config.height};
	int ret = 0;
	GxAlphaType alpha_type = GX_ALPHA_GLOBAL;

	ret = GxGDI_HardwareLayerRegister(layer_id, gui_core->osd_surface);
	if(0 != ret) {
		return;
	}
	ret = GxGDI_HardwareLayerColorKey(layer_id, gui_core->config.osd_trans);
	if(0 != ret) {
		return;
	}

	if(GX_COLOR_FMT_RGB565 == get_color_format(0, gui_core->config.bpp)) {
		alpha_type = GX_ALPHA_GLOBAL;
	} else {
		alpha_type = GX_ALPHA_PIXEL;
	}
	ret = GxGDI_HardwareLayerAlpha(layer_id, alpha_type, gui_core->config.osd_alpha);
	if(0 != ret) {
		return;
	}

	GXGDI_Lock();
	surface = GxGDI_HardwareLayerGet(layer_id);
	if(NULL == surface) {
		return;
	}
	ret = GXGDI_FillRect(surface, &rect, gui_core->config.osd_trans);
	if(0 != ret) {
		return;
	}
	GXGDI_Unlock();
}

#else /*NO_OS*/
#include "mini_gui_private.h"
#include "gal.h"
#include "gal_blit.h"
#include "hd_gal.h"
#endif /*NO_OS*/

int gdi_begin(void)
{
	return (hd_new_blit_list());
}

int gdi_end(void)
{
	return (hd_end_blit_list());
}
int gdi_update(void)
{
	return (hd_update_blit());
}


int gdi_commit(void)
{
	return (hd_commit_blit());
}
int gdi_enable(int flag)
{
	return (hd_enable_osd(flag));
}

int gxgdi_virtual_init(GuiCore *gui_core)
{
#ifndef NO_OS
	GAL_Rect rect = {0};
	GXGDI_Rect layer_rect = {0};
	char *layer_name = NULL;

	if(NULL == gui_core)
	{
		return (1);
	}

	layer_rect.x = rect.x = 0;
	layer_rect.x = rect.y = 0;
	layer_rect.w = rect.w = gui_core->config.width;
	layer_rect.h = rect.h = gui_core->config.height;
	gui_core->osd_surface = hd_get_surface(&rect, gui_core->config.bpp, NULL, GUI_FALSE);
	if(NULL == gui_core->osd_surface)
	{
		return (1);
	}
	if((gui_core->config.hardware_layer) &&
	   (GXAV_ID_SIRIUS == gui.config.chip_id)) {
		if(NULL == GxGDI_HardwareLayerGet(GX_LAYER_SOSD)) {
			_set_hardlayer(GX_LAYER_SOSD, gui_core);
			layer_name = GUI_STRDUP("sosd");
			gui_core->config.layer_id = GX_LAYER_SOSD;
		} else if(NULL == GxGDI_HardwareLayerGet(GX_LAYER_SOSD2)) {
			_set_hardlayer(GX_LAYER_SOSD2, gui_core);
			layer_name = GUI_STRDUP("sosd2");
			gui_core->config.layer_id = GX_LAYER_SOSD2;
		} else {
			gxlogd("[GUI] There is no hardware layer, switch to virtuallayer");
			goto VIRTUALLAYER;
		}
	} else {
VIRTUALLAYER:
		gui_core->config.hardware_layer = GUI_FALSE;
		layer_name = GUI_STRDUP(GxGDI_LayerRegister(gui_core->osd_surface, gui_core->config.x, gui_core->config.y));
		if(NULL == layer_name)
		{
			return (1);
		}
		GXGDI_LayerClear(layer_name, &layer_rect);
	}

	gui_core->layer_name = layer_name;
#endif /*NO_OS*/

	return (0);
}

int gxgdi_virtual_destroy(GuiCore *gui_core)
{
	status_t ret = GXCORE_SUCCESS;
	GuiSurface *surface = NULL;

#ifndef NO_OS
	if(gui_core->layer_name)
	{
		if(gui_core->config.hardware_layer == GUI_TRUE) {
			GxGDI_HardwareLayerEnable(gui_core->config.layer_id, GUI_FALSE);
			surface = GxGDI_HardwareLayerGet(gui_core->config.layer_id);
			if(surface) {
				hd_free_surface(surface);
			}
			GUI_FREE(gui_core->layer_name);
		} else {
			surface = GxGDI_LayerGetSurface(gui_core->layer_name);
			ret = GxGDI_LayerUnregister(gui_core->layer_name);
			if(surface) {
				gdi_lock();
				hd_free_surface(surface);
				gdi_unlock();
			}
		}
	}
#endif /*NO_OS*/

	if(ret != GXCORE_SUCCESS)
		return (1);
	else
		return (0);
}

int gdi_flip(void)
{
#ifndef NO_OS
	return (hd_flip());
#else
	return (0);
#endif /*NO_OS*/
}

static handle_t gdi_sem_id;
void gdi_wait(void)
{
	if(0 == gdi_sem_id) {
		GxCore_SemCreate(&gdi_sem_id, 0);
	}

	if(gdi_sem_id) {
		GxCore_SemWait(gdi_sem_id);
	}

	GxCore_SemDelete(gdi_sem_id);
	gdi_sem_id = 0;
}

void gdi_post(void)
{
	if(gdi_sem_id) {
		GxCore_SemPost(gdi_sem_id);
	}
}


