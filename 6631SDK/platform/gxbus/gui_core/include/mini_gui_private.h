#ifndef __MINI_GUI_PRIVATE_H__
#define __MINI_GUI_PRIVATE_H__

#include "gxtype.h"
#include "gdi_core.h"
#include "gui_core.h"
#include "gui_hash.h"
#include "stack.h"
#include "av/avapi.h"
#include "widget.h"
#include "common/gxmalloc.h"
#include "gxos/gxcore_os_core.h"
#include "gxos/gxcore_os_filesys.h"

typedef struct _MiniGUIConfig
{
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	GuiSurface *surface;
	GuiSurface *logo_surface;
	hash_t *font;
	hash_t *image;
	hash_t *signal_hash;
	gx_stack_t *win_stack;
	char *current_font;

	/*GUI configure*/
	char *theme_path;
	GuiWidget *root_widget;
	GuiWidget *focus_widget;
}MiniGUIConfig;

#ifdef NO_OS
#define GUI_STRDUP(str)   GxCore_Strdup(str)
#define GUI_MALLOC(size)  GxCore_Malloc(size)
#define GUI_MALLOCZ(size) GxCore_Mallocz(size)
#define GUI_REALLOC(ptr, size) GxCore_Realloc(ptr, size)
#define GUI_FREE(p) if(p) {GxCore_Free(p); p = NULL;}
#endif

#define GUI_TRUE            (1)
#define GUI_FALSE           (0)

extern GuiSurface *screen;
extern GuiSurface *spp_screen;
extern int g_device_handle;
extern int vpu_handle;
extern int jpeg_handle;
GuiCore *GUI_GetCurrent(void);

int gdi_begin(void);
int gdi_end(void);
int gdi_commit(void);

MiniGUIConfig minigui_config;
/*XML*/
status_t minigui_xml_parse(char *path);

GuiWidgetOps frame_ops;
GuiWidgetOps label_ops;
GuiWidgetOps edit_box_ops;
GuiWidgetOps progress_ops;
GuiWidgetOps list_ops;

#endif /*__MINI_GUI_PRIVATE_H__*/
