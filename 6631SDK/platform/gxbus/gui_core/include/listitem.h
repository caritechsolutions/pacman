#ifndef __LISTITEM_H__
#define __LISTITEM_H__

#include "gui_private.h"
#include "widget.h"

typedef struct _GuiListItem
{
	GuiWidget *base;
	void *content;
	char *font;
	int alignment;
	int pos;
	int i18n;
	int time;
	int wait_time;
	int postpone_time;
	int enable_roll;
	bool arrange_right;
	image_desc *image[3];
	image_desc *active_img;
	image_desc *image_g[3][3];
	image_desc *active_img_g[3];
	void *surface;
	Color active_color;
	GuiWidget *link;
	event_list *timer;
}GuiListItem;

extern GuiWidgetOps listitem_ops;

#endif

