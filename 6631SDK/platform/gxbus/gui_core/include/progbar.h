#ifndef __PROGBAR_H__
#define __PROGBAR_H__

#include "gui_private.h"
#include "widget.h"

enum
{
	PROGBAR_PERCENTAGE,
	PROGBAR_DECIMAL,
	PROGBAR_HIDE
};

enum
{
	PROGBAR_NORMAL,
	PROGBAR_WATER,
	PROGBAR_VERTICAL
};

typedef struct _GuiProgbar
{
	GuiWidget *base;
	int text_format;
	int minimum;
	int maximum;
	int old_value;
	int value;
	int format;
	int text_position;
	void *back_surface;
	void *fore_surface;
	image_desc *back_img[3];
	image_desc *fore_img[3];
	char *font;
	char *string;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *reachend_event;
}GuiProgbar;

extern GuiWidgetOps progbar_ops;

#endif

