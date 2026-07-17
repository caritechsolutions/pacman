#ifndef __GAL_H__
#define __GAL_H__

#include "gxcore.h"
#include "gdi_core.h"
#include "gui_core.h"

__BEGIN_DECLS

typedef struct _GAL_Interval{
	short horizontal;
	short vertical;
}GAL_Interval;

#ifndef NO_OS
typedef struct _IMG_Config {
	int threshold_size;
	int level;
	int level_size;
} IMG_Config;

typedef struct _GALMemory {
	ImageMemorySource source;
	union {
		GxHwMallocObj *vq;
		void *ptr;
	}block;
	int size;
} GALMemory;

IMG_Config image_config;

ImageMemorySource gal_get_memory_type(int size);
GALMemory *gal_alloc_memory_by_source(int size, ImageMemorySource source);
GALMemory *gal_alloc_memory(int size);
int gal_free_memory(GALMemory *p);
#endif

int gal_check_image_size(int size);

int gdi_flip(void);
int gxgdi_init(void);
int gxgdi_virtual_init(GuiCore *gui_core);
int gxgdi_virtual_destroy(GuiCore *gui_core);

int gdi_change_surface(void **src0, void **src1);

int gdi_update(void);
void gal_putpixel(void *screen, int x, int y, unsigned int pixel);
int gal_fillrect(void *screen, GAL_Rect * dstrect, unsigned int color);
int gal_color2index(void *screen, int color);
int gal_color2index0(void *screen, int color);
void gal_set_gui_transparent(unsigned int color);
int gal_get_gui_transparent(void);

int gal_stretch_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect);

int gal_set_osd_transparent(unsigned int color, unsigned short opacity);

void gxgdi_updatelayers(GuiSurface *swap_surface);

int gal_add_palette(GuiConfig* config, const char *pal_name);
status_t check_layer_enable(void);

void gdi_wait(void);
void gdi_post(void);

char *GxGDI_LayerRegisterNolock(GuiSurface *surface, unsigned int x, unsigned int y);
status_t GxGDI_LayerUnregisterNolock(const char *name);

__END_DECLS

#endif

