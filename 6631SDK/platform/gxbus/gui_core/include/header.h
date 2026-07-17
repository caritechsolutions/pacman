#ifndef __HEADER_H__
#define __HEADER_H__

#include "gui_private.h"
#include "widget.h"

enum
{
	HEADER_SHOW,
	HEADER_HIDE
};

typedef struct _Header_Para
{
	int index;
	int xPos;
	int xSize;
	int alignment;
	int automatic;
	int string_wrap;
	int disable_mirror;
}Header_Para;

typedef struct _GuiHeader
{
	GuiWidget *base;
	int visible;
	int num_col;
	int offset;
	//int grid_width;
}GuiHeader;

extern GuiWidgetOps header_ops;

#endif

