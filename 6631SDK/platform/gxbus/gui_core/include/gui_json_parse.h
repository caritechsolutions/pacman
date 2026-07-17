#ifndef __GUI_JSON_PARSE_H__
	#define __GUI_JSON_PARSE_H__

#include "JSON_parser.h"
#include "stack.h"


#define JSON_FILE_KEYMAP			"file_keymap"
#define JSON_FILE_WIDGET			"file_widget"
#define JSON_FILE_COLOR             "file_color"
#define JSON_FILE_STYLE			"file_style"
#define JSON_FILE_I18N			"file_i18n"
#define JSON_FILE_IMAGE			"file_image"
#define JSON_FILE_FONT			"file_font"
#define JSON_FILE_PAL				"file_pal"
#define JSON_GA					"ga"
#define JSON_ANTIFLICKER			"antiflicker"

#define JSON_LANGUAGE			"language"
#define JSON_IMAGE				"image"
#define JSON_NAME				"name"
#define JSON_FILE					"file"
#define JSON_SIZE					"size"
#define JSON_KEY					"key"

#define JSON_WIDGET				"widget"
#define JSON_FREEDOM				"freedom_widget"
#define JSON_INTERFACE			"interface"

#define JSON_WIDTH				"width"
#define JSON_HEIGHT				"height"
#define JSON_BPP					"bpp"
#define JSON_MULTCLUT			"bpp8_multclut"
#define JSON_OSD_TRANS			"osd_trans"
#define JSON_GUI_TRANS			"gui_trans"
#define JSON_OSD_ALPHA			"osd_alpha_global"
#define JSON_ALPHA_COLOR			"osd_alpha_color"

#define JSON_PROPERTY			"property"
#define JSON_SIGNAL				"signal"
#define JSON_CHILDREN				"children"
#define JSON_CLASS				"class"
#define JSON_STYLE				"style"

/*Create*/
int gui_json_config(void* ctx, int type, const JSON_value* value);
int gui_json_keymap(void* ctx, int type, const JSON_value* value);
int gui_json_i18n(void* ctx, int type, const JSON_value* value);
int gui_json_images(void* ctx, int type, const JSON_value* value);
int gui_json_font(void* ctx, int type, const JSON_value* value);
int gui_json_widget(void* ctx, int type, const JSON_value* value);
int gui_json_style(void* ctx, int type, const JSON_value* value);
int gui_json_color(void* ctx, int type, const JSON_value* value);

/*Free*/
int gui_json_font_freedata(void* ctx, int type, const JSON_value* value);
status_t gui_json_keymap_free(void);
status_t gui_json_widget_freedata(GuiCore* gui_core);
status_t gui_json_style_freedata(GuiWidget* style_widget);
status_t gui_json_i18n_freedata(GuiConfig* cfg);
status_t gui_json_image_freedata(GuiImgs* img);
status_t gui_json_config_freedata(GuiConfig* cfg);
status_t gui_json_color_free(void);

status_t gui_json_main(const char *config_file, JSON_parser_callback cb, void *ptr);

#endif


