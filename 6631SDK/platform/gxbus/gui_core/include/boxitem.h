#ifndef __BOXITEM_H__
#define __BOXITEM_H__

#include "gui_private.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct _GuiBoxItem
{
	GuiWidget *base;
	GuiWidget *focused;
	int active;
	GuiSurface *surface;
	image_desc *image[3];
	image_desc *image_g[3][3];
}GuiBoxItem;

extern GuiWidgetOps boxitem_ops;

__END_DECLS

#endif



