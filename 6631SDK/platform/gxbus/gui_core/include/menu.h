#ifndef __MENU_H__
#define __MENU_H__

#include "gui_private.h"

typedef enum _MENU_DIRECTION
{
    DIRECTION_SILL,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
}MENU_DIRECTION;

typedef enum _MENU_FORMAT
{
	MENU_HORIZONTAL_UP,
	MENU_HORIZONTAL_DOWN,
	MENU_VERTICAL_LEFT,
	MENU_VERTICAL_RIGHT
}MENU_FORMAT;

typedef struct _GuiMenu
{
	GuiWidget* base;
	MENU_FORMAT format;
	MENU_DIRECTION direction;
	unsigned int fixed_list;
	unsigned char *focused_list;
	unsigned int visible_list;
	void *back_surface;
	void *movable_surface;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *clicked_event;
}GuiMenu;

extern GuiWidgetOps menu_ops;

#endif

