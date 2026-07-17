#ifndef __BOX_H__
#define __BOX_H__

#include "gui_private.h"

enum
{
	BOX_SHOW_SCROLLBAR,
	BOX_HIDE_SCROLLBAR
};

enum
{
	BOX_VERTICAL,
	BOX_HORIZONTAL
};

typedef struct _GuiBox
{
	GuiWidget* base;
	GuiWidget* focus;
	image_desc *left_arrow;
	image_desc *right_arrow;
	int update_all;
	int item_interval;
	int format;
	int direction;
	int show_scrollbar;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *clicked_event;
}GuiBox;

extern GuiWidgetOps box_ops;

#endif

