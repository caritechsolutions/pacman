#ifndef __TIMEITEM_H__
#define __TIMEITEM_H__
#include "gui_private.h"

typedef struct _GuiTimeitem
{
	GuiWidget* base;
	char *font;
	int alignment;
	int grid_width;
	int current_select;
	int current_item;/*Focused item arrange in horizon*/
	int focus_item;
	int max_index;   /*max focus_item*/
	int total_items;
	GUI_Rect focus_rect;
	image_desc *unfocus_img[3];
	image_desc *focus_img[3];
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *getdata_event;
}GuiTimeitem;

extern GuiWidgetOps timeitem_ops;

#endif /*__TIMEITEM_H__*/

