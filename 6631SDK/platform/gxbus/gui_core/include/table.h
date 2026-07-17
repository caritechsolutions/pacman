#ifndef __TABLE_H__
#define __TABLE_H__

#include "gui_private.h"
#include "widget.h"
#include "gui_timer.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct _GuiTable
{
	GuiWidget *base;
	unsigned int row_num;
	unsigned int column_num;
	unsigned int visible_row;
	unsigned int interval;
	unsigned int updata_all;
	unsigned int total_num;
	GuiWidget *focused_child;
	GuiWidget *first_child;
	GuiWidget *scrollbar;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *keypress_event;
}GuiTable;


extern GuiWidgetOps table_ops;

__END_DECLS

#endif    /*__TABLE_H__*/


