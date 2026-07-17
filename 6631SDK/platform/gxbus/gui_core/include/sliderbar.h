#ifndef __SLIDEBAR_H__
#define __SLIDEBAR_H__

#include "widget.h"
#include "gui_private.h"

#include "gxcore.h"

__BEGIN_DECLS

typedef enum
{
	HORIZONTAL_NO_SCALE,
	VERTICAL_NO_SCALE,
	HORIZONTAL_SCALE,
	VERTICAL_SCALE
}SliderbarFormat;

typedef enum
{
	SLIDERBAR_NORMAL,
	SLIDERBAR_NULL,
	SLIDERBAR_WATER
}SliderbarMode;

typedef struct _GuiSliderbar
{
	GuiWidget *base;
	image_desc *back_img[3];
	image_desc *fore_img[3];
	image_desc *cursor_img;
	SliderbarFormat format;
	SliderbarMode mode;
	int min;
	int max;
	int old_value;
	int value;
	int step;
	void *surface;  // Transparent effect
	void *back_surface;
	void *fore_surface;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *keypress_event;
}GuiSliderbar;

extern GuiWidgetOps sliderbar_ops;

__END_DECLS

#endif  /*__SLIDEBAR_H__*/

