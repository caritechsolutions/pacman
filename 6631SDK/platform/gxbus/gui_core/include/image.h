#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "gui_private.h"
#include "widget.h"

typedef struct _ComImage
{
	image_desc *image_lt;
	image_desc *image_rt;
	image_desc *image_lb;
	image_desc *image_rb;
	image_desc *image_l;
	image_desc *image_r;
	image_desc *image_t;
	image_desc *image_b;
}ComImage;

enum
{
	IMG_SINGLE,
	IMG_MULTIPLE
};

typedef struct _GuiImage
{
	GuiWidget *base;
	int opacity;
	int mode;
	char *filepath;
	union{
		image_desc *single_img;
		ComImage com_img;
	}p;
	GuiSurface *surface;
	GuiSurface *map_surface /*Map a surface snap*/;
	event_list *timer;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
}GuiImage;

extern GuiWidgetOps image_ops;

#endif

