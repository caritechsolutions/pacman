#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "widget.h"
#include "gui_private.h"

typedef struct _GuiButton {
	GuiWidget *base;
	char *font;
	char *string;
	image_desc *image[3];
	image_desc *image_g[3][3];
	int alignment;
	GuiWidget *link;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *keypress_event;
}GuiButton;

extern GuiWidgetOps button_ops;

#endif

