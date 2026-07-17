#ifndef __GUI_SCRIPT_H__
#define __GUI_SCRIPT_H__

status_t gui_script_parse(GuiCore* gui_core, const char *config_file);
status_t gui_script_parse_freedata(GuiCore *gui_core);

status_t gui_resource_parse(const char *name, GuiCore *gui_core, const char *config_file);
status_t gui_resource_parse_freedata(const char *name, GuiCore *gui_core);

#endif

