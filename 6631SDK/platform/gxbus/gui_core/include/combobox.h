#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

#include "gui_private.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct _GuiComboBox
{
	GuiWidget* base;
	char *font;
	GuiSurface *surface;
	image_desc *image[3];
	image_desc *image_g[3][3];
	image_desc *left_arrow[3];
	image_desc *right_arrow[3];
	int cur_sel;
	int total;
	int update_surface;
	char **content;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *change_event;
}GuiComboBox;

extern GuiWidgetOps combobox_ops;

__END_DECLS

#endif
