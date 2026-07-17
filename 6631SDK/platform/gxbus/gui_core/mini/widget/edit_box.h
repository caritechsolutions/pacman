#ifndef __MINI_Edit_box_H__
#define __MINI_Edit_box_H__

#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

typedef struct _GuiEdit_box {
	GuiWidget *base;
	int count;
	int start_index;
	int update;
	int update_index[10];
	int cursor_pos;
	hash_t *edit_box_data;
	GAL_Rect *rect_array;
}GuiEdit_box;

#endif /*__MINI_Edit_box_H__*/

