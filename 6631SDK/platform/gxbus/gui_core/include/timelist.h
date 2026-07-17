#ifndef __TIMELIST_H__
#define __TIMELIST_H__

#include "gui_private.h"


#define                   FLOP_MOVE                 (0)
#define                   GLID_MOVE                 (1 << 0)
#define                   GLID_UPDATE               (1 << 1)

typedef struct _TimeData
{
    signed int day_offset;
    signed int hour;
    signed int minute;
}TimeData;

typedef enum _TimitemStringImage
{
	LAY_OVERLAP,
	LAY_TILE
}TimitemStringImage;

typedef struct _Timeitem_node
{
	int series_number;
	TimeData start;
	TimeData end;
	char *string;
	int fore_color;
	int back_color;
	char *font;
	image_desc *image;
	GAL_Rect rect;
	TimitemStringImage laymode;
}Timeitem_node;

typedef enum _TimelistDirection
{
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    TIMELIST_UPDATE,
    TIMELIST_STILL,
    TIMELIST_CREATE
}TimelistDirection;

typedef enum _TimelistArrange {
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT
} TimelistArrange;

typedef struct _GuiTimelist
{
	GuiWidget* base;
	char *font;
	int alignment;
	int total;
	int total_col;
	int visible_row;
	int current_row;
	int grid_width;
	int head_height;
	int time_interval;
	int time_offset;
	int last_x;
	int item_pos;
	int item_height;
	handle_t timelist_mutex;
	TimelistArrange arrange_format;
	int format;
	TimelistDirection direction;
	void *surface;
	GuiSurface *back_surface;
	GuiWidget *focus;
	TimeData start_time;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *getdata_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *show_event;
}GuiTimelist;

extern GuiWidgetOps timelist_ops;

#endif /*__TIMELIST_H__*/

