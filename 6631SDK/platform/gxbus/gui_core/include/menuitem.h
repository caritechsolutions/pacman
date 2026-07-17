#ifndef __MENUITEM_H__
#define __MENUITEM_H__

#include "gui_private.h"

typedef struct _GuiMenuItem
{
	GuiWidget* base;
	int alignment;
	char *string;
	char *font;
	image_desc *back_image[3];
	GuiWidget *link;
}GuiMenuItem;

extern GuiWidgetOps menuitem_ops;

#endif

