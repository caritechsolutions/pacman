#ifndef __GUI_PRIVATE_H__
#define __GUI_PRIVATE_H__

#include <gxtype.h>

#include <stdbool.h>
#include <stdint.h>

#include "gui_key_test.h"

#include "gxsecure/gxsecure_api.h"
#ifndef NO_OS
#include "gui_config.h"
#endif /*NO_OS*/
#include "gdi_core.h"
#include "gui_core.h"
#include "gui_debug.h"
#include "color.h"
#include "stack.h"

#ifdef LINUX_OS
#include "SDL.h"
#endif

__BEGIN_DECLS

enum {
	GUI_UNFOCUS,
	GUI_FOCUS,
	GUI_DISABLE
};
#ifndef NO_OS
unsigned char gui_poll_event(GUI_Event *data);
unsigned char gui_poll_event(GUI_Event *data);
VirtualGUIMode get_virtual_gui_mode(char *value);
extern GuiSignalTable* signal_table;
int gui_set_mirror0(int flag);
int gui_set_mirror_flag(int flag);
void revert_gui_key(GUI_Event* event);
#endif /*NO_OS*/
GuiCore *GUI_GetCurrent(void);

extern GuiSurface *screen;
extern GuiSurface *back_screen;
extern GuiSurface *spp_screen;

extern GuiCore gui;

extern int g_device_handle;
extern int vpu_handle;
extern int jpeg_handle;
extern int vdec_handle;

//#define JPEG_LOAD

/*Text alignment flags, horizontal*/
#define GUI_TA_HORIZONTAL		(3<<0)
#define GUI_TA_LEFT			(0<<0)
#define GUI_TA_RIGHT			(1<<0)
#define GUI_TA_HCENTRE			(2<<0)

/*Text alignment flags, vertical*/
#define GUI_TA_VERTICAL	(3<<2)
#define GUI_TA_TOP		(0<<2)
#define GUI_TA_BOTTOM	(1<<2)
#define GUI_TA_BASELINE	(2<<2)
#define GUI_TA_VCENTRE	(3<<2)

#define GUI_TRUE			(1)
#define GUI_FALSE			(0)

__END_DECLS

#endif

