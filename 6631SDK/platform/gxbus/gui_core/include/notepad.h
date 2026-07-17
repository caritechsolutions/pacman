#ifndef __NOTEPAD_H__
#define __NOTEPAD_H__

#include "gui_private.h"


#define FILE_MAX_BUFFER         (1024)
#define FILE_BUFFER_LIMIT	(4 * 1024)

#define FILE_DIRECTION_NO	(0)
#define FILE_DIRECTION_UP	(1)
#define FILE_DIRECTION_DOWN	(2)

#define MAX_ACTIVE_LINE         (10)

typedef struct _GuiNotepad{
	GuiWidget *base;
	char *file_path;
	char *string;
	char *arabic_string;
	char *font;
	image_desc *img_desc;
	int *line_pos;
	int line_index;
	int temp_line;
	handle_t file;
	int delay;
	int alignment;
	int vertical_margin;
	int horizontal_margin;
	int vinterval;
	int hinterval;
	int text_offset;

	int file_size;
	int buffer_size;
	int file_position;
	int buffer_position;
	int direction;

	int total_num;
	int visible_lines;
	int line_num;
	int autohide_scroll;
	int active_line;
	int active_array[MAX_ACTIVE_LINE];
	int epg_flag;
	int save_context;
	bool adjust_string;
	event_list *timer;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
}GuiNotepad;


extern GuiWidgetOps notepad_ops;

#endif

