#ifndef __WINDOW_H__
#define __WINDOW_H__
#include "gui_private.h"
#include "gui_core.h"

enum
{
	FORMAT_DIALOG = 0,
	FORMAT_POPUP,
	FORMAT_MOVABLE
};

typedef struct _GuiWindow
{
	GuiWidget *base;
	GuiWidget *active_widget;
	image_desc *image;
	image_desc *back_desc;
	GUI_Rect *surface_rect;
	void *surface;
	void *movable_surface;
	GuiSurface *back_up_surface;
	int update_back;
	int surface_move;
	int format;
	int update_all;
	int move_origin_x;
	int move_origin_y;
	int popup_recorded;
	int back_sync;
	int need_fade;
	int fade_steps;
	ImagePlayMode fade_mode;
	char *back_ground;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *close_event;
	GuiWidgetSignal *show_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *service_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *got_focus;
	GuiWidgetSignal *lost_focus;
}GuiWindow;

extern GuiWidgetOps window_ops;

#endif

