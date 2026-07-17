#ifndef __FILE_IMAGE_H__
    #define __FILE_IMAGE_H__

#include "gui_private.h"
#include "widget.h"

typedef struct _ComFileImage
{
	image_desc *image_lt;
	image_desc *image_rt;
	image_desc *image_lb;
	image_desc *image_rb;
	image_desc *image_l;
	image_desc *image_r;
	image_desc *image_t;
	image_desc *image_b;
}ComFileImage;

enum{
	FILE_IMG_SINGLE,
	FILE_IMG_MULTIPLE,
	FILE_IMG_DYNAMIC
};

typedef struct _GuiFileImage
{
    GuiWidget *base;
    int opacity;
    int mode;
    GuiSurface *surface;
    GuiSurface *snap_surface;
    union{
	image_desc *single_img;
	ComFileImage com_img;
    }p;
    int alu_mode;
    event_list *timer;
    GuiWidgetSignal *create_event;
    GuiWidgetSignal *destroy_event;
}GuiFileImage;

extern GuiWidgetOps file_image_ops;

#endif


