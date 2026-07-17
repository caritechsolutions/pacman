#ifndef __LISTVIEW_H__
#define __LISTVIEW_H__
#include "gui_private.h"
#include "widget.h"

#define LISTVIEW_SHOW_SCROLL			(1)
#define LISTVIEW_HIDE_SCROLL			(0)
#define LISTVIEW_SHOW_HEADER			(1 << 1)
#define LISTVIEW_HIDE_HEADER			(0 << 1)
#define LISTVIEW_PAGE				(1 << 2)
#define LISTVIEW_FIX_ROUND			(1 << 3)

typedef enum _ListviewArrange {
	ARRANGE_LEFT_TO_RIGHT,
	ARRANGE_RIGHT_TO_LEFT
}LISTVIEW_ARRANGE;

typedef struct _ListViewItem
{
	int min;
	int num;
	char ** string;
}ListViewItem;

typedef struct _ListViewImage
{
	char *id;
	image_desc *img_desc;
}ListViewImage;

typedef enum _LISTVIEW_UPDATE_STATE {
	ALL_UPDATE,
	ITEM_UPDATE,
	ROLL_UP_UPDATE,
	ROLL_DOWN_UPDATE,
	NO_UPDATE
} LISTVIEW_UPDATE_STATE;

typedef struct _GuiListView
{
	GuiWidget *base;
	int format;
	int cur_sel;
	int focus_sel;
	int active_index;
	int total_num;
	int real_total;
	int visible_row;
	int fix_select;
	int i18n;
	int interval;
	int time;
	int time_out;
	int enable_roll;
	int disable_accelerate;
	LISTVIEW_ARRANGE arrange;
	LISTVIEW_UPDATE_STATE update_state;
	GuiSurface *surface;
	image_desc *item_img[3];
	image_desc *item_img_g[3][3];
	Color active_color;
	image_desc *active_img;
	image_desc *active_img_g[3];
	ListViewItem item;
	GuiWidget *focus_item;
	char *font;
	event_list *timer;
	event_list *start_timer;
	GuiWidgetSignal *getdata_event;
	GuiWidgetSignal *gettotal_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *timeout_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *draw_event;
	GuiWidgetSignal *destroy_event;
}GuiListView;

extern GuiWidgetOps listview_ops;

#endif

