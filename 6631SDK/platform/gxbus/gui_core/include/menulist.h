#ifndef __MENULIST_H__
#define __MENULIST_H__

#include "gui_private.h"

typedef struct _GuiMenuList
{
    GuiWidget* base;
    int alignment;
    int trigger;
    int width;
    int height;
    image_desc *image[3];
    image_desc *item_image[3];
    GuiWidget *focus_widget;
    char *font;
    char *string;
    void *surface;
    void *back_surface;
}GuiMenuList;

extern GuiWidgetOps menulist_ops;

#endif /*__MENULIST_H__*/

