#ifndef __GUI_XMLPARSE_H__
#define __GUI_XMLPARSE_H__

#include "gxtype.h"
#include "parser.h"
#include "gui_hash.h"
#include "gui_core.h"
#include "widget.h"
#include "gxcore.h"
#include "memmgr.h"

__BEGIN_DECLS

enum {
    GX_ERROR_XML_DOC_PARSE = 3,
    GX_ERROR_XML_DOC_EMPTY,
    GX_ERROR_CLUT_INVALID,
    GX_ERROR_FILE_WIDGET_INVALID,
    GX_ERROR_FILE_STYLE_INVALID,
    GX_ERROR_FILE_I18N_INVALID,
    GX_ERROR_FILE_IMAGE_INVALID,
    GX_ERROR_FILE_FONT_INVALID,
    GX_ERROR_FILE_PAL_INVALID
};


#define GUI_CONFIG_STATE(ret, str) if(str && (0 == strcasecmp("enable", (const char *)str)))ret = 1;else ret = 0;
#define GUI_XML_Printf(...) gxlogd(__VA_ARGS__)

typedef unsigned char xmlChar;

#define XML_FREE(x) if(x){GXxmlFree(x);x=NULL;}
#define XML_GET_CHILD(node) node->childs
#define XML_GET_ROOT(doc)   doc->root


#define ID_STR              ((xmlChar *)"id"       )
#define NAME_STR            ((xmlChar *)"name"     )
#define VALUE_STR           ((xmlChar *)"value"    )
#define WIDGET_STR          ((xmlChar *)"widget"   )
#define CLASS_STR           ((xmlChar *)"class"    )
#define STYLE_STR           ((xmlChar *)"style"    )
#define PROPERTY_STR        ((xmlChar *)"property" )
#define SIGNAL_STR          ((xmlChar *)"signal"   )
#define HANDLER_STR         ((xmlChar *)"handler"  )
#define CHILDREN_STR        ((xmlChar *)"children" )
#define FILES_STR           ((xmlChar *)"files"    )
#define FONT_STR            ((xmlChar *)"font"     )
#define FONT_THRESHOLD_STR  ((xmlChar *)"threshold_size")
#define IMAGE_STR           ((xmlChar *)"image"    )
#define DEFAULT_PAL_STR	    ((xmlChar *)"default_palette")
#define COLOR_STR           ((xmlChar *)"color"    )
#define FILE_STR            ((xmlChar *)"file"     )
#define FILENAME_STR        ((xmlChar *)"filename" )
#define PATH_STR            ((xmlChar *)"path"     )
#define I18N_STR            ((xmlChar *)"i18n"     )
#define FONTS_STR           ((xmlChar *)"fonts"    )
#define SEARCH_STR          ((xmlChar *)"search"   )
#define THEMES_STR          ((xmlChar *)"themes"   )
#define THEME_STR           ((xmlChar *)"theme"    )
#define TRAN_STR            ((xmlChar *)"tran"     )
#define LANGUAGE_STR        ((xmlChar *)"language" )
#define INTERFACE_STR       ((xmlChar *)"interface")
#define FLAG_STR            ((xmlChar *)"flag")
#define FREEDOM_WIDGET_STR  ((xmlChar *)"freedom_widget"  )

//config
#define PRJ_PATH_STR      ((xmlChar *) "prj_path"         )
#define FILE_KEYMAP_STR   ((xmlChar *) "file_keymap"      )
#define FILE_WIDGET_STR   ((xmlChar *) "file_widget"      )
#define FILE_STYLE_STR    ((xmlChar *) "file_style"       )
#define FILE_COLOR_STR    ((xmlChar *) "file_color"       )
#define FILE_I18N_STR     ((xmlChar *) "file_i18n"        )
#define FILE_IMAGE_STR    ((xmlChar *) "file_image"       )
#define FILE_FONT_STR     ((xmlChar *) "file_font"        )
#define KEY_MODE		  ((xmlChar *) "key_mode"		  )
#define MESSAGE_MODE	  ((xmlChar *) "message_mode"	  )
#define	X_POSITION		  ((xmlChar *) "x")
#define	Y_POSITION		  ((xmlChar *) "y")
#define FILE_PAL_STR      ((xmlChar *) "file_pal"         )
#define WIDTH_STR         ((xmlChar *) "width"            )
#define HEIGHT_STR        ((xmlChar *) "height"           )
#define BPP_STR           ((xmlChar *) "bpp"              )
#define CACHE_WIDGETS_STR           ((xmlChar *) "cache_widgets"              )
#define CACHE_CONTEXT_STR           ((xmlChar *) "cache_context")
#define MIRROR_STR        ((xmlChar *) "mirror")
#define KEYPRESS_REVERT_STR         ((xmlChar *) "keypress_revert")
#define BPP8_MULTCLUT_STR ((xmlChar *) "bpp8_multclut"    )
#define GA_STR            ((xmlChar *) "ga"               )
#define GA_COUNT          ((xmlChar *) "ga_count"         )
#define ENCODING          ((xmlChar *) "encoding"         )
#define FOR_GDI           ((xmlChar *) "for_gdi")
#define OSD_SPP_SHARE     ((xmlChar *) "osd_spp_share")
#define IMAGE_LIMIT       ((xmlChar *) "image_limit"      )
#define ANTIFLICKER_STR   ((xmlChar *) "antiflicker"      )
#define DOUBLE_BUFFER_STR ((xmlChar *) "double_buffer"    )
#define HARDWARE_LAYER_STR ((xmlChar *) "hardware_layer"  )
#define FONT_OVERLAY_STR  ((xmlChar *)"font_overlay"      )
#define FONT_INPOOL_STR   ((xmlChar *)"font_in_pool"      )
#define KEY_SHIELD_STR	  ((xmlChar *) "key_shield"	  )
#define SCHEDULE_TIME_STR ((xmlChar *) "schedule_time"	  )
#define ESOTERIC_LANGUAGE ((xmlChar *) "esoteric_language")
#define PRELOAD_STR       ((xmlChar *) "preload")
#define BUFFER_SIZE_STR   ((xmlChar *) "buffer_size"      )
#define BLOCK_JOIN        ((xmlChar *) "block_join"       )
#define OSD_TRANS_STR     ((xmlChar *) "osd_trans"        )
#define GUI_TRANS_STR     ((xmlChar *) "gui_trans"        )
#define OSD_ALPHA_GLOBAL  ((xmlChar *) "osd_alpha_global" )
#define OSD_ALPHA_COLOR   ((xmlChar *) "osd_alpha_color"  )
#define COLOR             ((xmlChar *) "color"            )
#define IMAGES_STR        ((xmlChar *) "images"           )
#define COLORS_STR        ((xmlChar *) "colors"           )
#define SIZE_STR          ((xmlChar *) "size"             )
#define KEYMAP_STR        ((xmlChar *) "keymap"           )
#define KEY_STR           ((xmlChar *) "key"              )
#define VIRTUALKEY_STR    ((xmlChar *) "virtualkey"       )
#define KEY_PROCESS	  ((xmlChar *) "keyprocess")
#define TREE_STR          ((xmlChar *) "tree"             )
#define ITEM_STR          ((xmlChar *) "item"             )
#define DISPLAY_STR       ((xmlChar *) "display"          )
#define GROUP_STR         ((xmlChar *) "group"            )
#define MUTEX_STR         ((xmlChar *) "mutex"            )
#define TRUE_STR          ((xmlChar *) "true"             )

status_t widget_quit(GuiWidget *widget);

status_t gui_xml_parse (GuiCore *gui_core, const char *config_file);
status_t gui_xml_config(GuiConfig *cfg, const char *filename);
status_t gui_xml_keymap(GuiConfig *config, const char *filename);
status_t gui_xml_widget(GuiConfig* config, const char *filename);
status_t gui_xml_style (GuiWidget *style_widget, const char *filename);
status_t gui_xml_i18n  (GuiConfig *cfg, const char *filename);
status_t gui_xml_image(GuiConfig* config, const char *filename);
status_t gui_xml_color (GuiColors *color, const char *filename);
status_t gui_xml_font(GuiConfig* config, const char *filename);

status_t gui_xml_parse_freedata (GuiCore* gui_core);
status_t gui_xml_config_freedata(GuiConfig* cfg);
status_t gui_xml_keymap_freedata(GuiConfig* config);
status_t gui_xml_widget_freedata(GuiConfig* cfg);
status_t gui_xml_style_freedata (GuiWidget* style_widget);
status_t gui_xml_i18n_freedata  (GuiConfig* cfg);
status_t gui_xml_image_freedata (void *data);
status_t gui_xml_color_freedata (GuiColors* color);
status_t gui_xml_font_freedata(GuiConfig *config);

status_t gui_xml_reload_widget(GuiCore* gui_core, const char *file_path);

char* gui_mallocpath_swapfile(const char* dir_file, const char* file_name);
status_t create_interface(GuiConfig *config, GXxmlNodePtr GXxml_node);

int connect_global_signal(GuiConfig *config, const char *name, unsigned char *content);

status_t gui_rsc_xml_parse (const char *name, GuiCore *gui_core, const char *config_file);

__END_DECLS

#endif /* __GUI_XMLPARSE_H__ */

