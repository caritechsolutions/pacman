#ifndef __MINI_LIST_H__
#define __MINI_LIST_H__

#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

typedef struct _GuiList {
	GuiWidget *base;
	int count;
	int start_index;
	int update;
	int update_index[10];
	int select;
	int cursor_pos;
	hash_t *list_data;
	GAL_Rect *rect_array;
}GuiList;

#endif /*__MINI_LIST_H__*/

