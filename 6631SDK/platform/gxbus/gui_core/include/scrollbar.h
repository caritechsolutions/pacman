#ifndef __SCROLLBAR_H__
#define __SCROLLBAR_H__

#include "gui_private.h"

enum
{
	SCROLL_SHOW,
	SCROLL_HIDE
};

typedef struct _GuiScrollbar
{
	GuiWidget* base;
	int format;
	image_desc *arrow_img[2];
	image_desc *back_img;
	image_desc *fore_img[3];
}GuiScrollbar;

extern GuiWidgetOps scrollbar_ops;

#endif

