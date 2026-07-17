#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "gui_private.h"
#include "widget.h"

typedef enum{
	CANVAS_NONE,
	CANVAS_PIXEL,
	CANVAS_RECTANGLE,
	CANVAS_IMAGE,
	CANVAS_STRING
}CanvasType;

/*typedef struct _GuiPaintList{
	CanvasType type;
	GUI_Rect update_rect;
	Color color;
	image_desc *image;
	struct _GuiPaintList *next;
}GuiPaintList;*/

typedef struct _GuiCanvas{
	GuiWidget *base;
	CanvasType type;
	char *font;
	GuiSurface *surface;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *show_event;
	GuiWidgetSignal *destroy_event;
	//GuiPaintList *paint_list;
}GuiCanvas;

extern GuiWidgetOps canvas_ops;

#endif
