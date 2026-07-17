#ifndef __GUI_I18N_H__
	#define __GUI_I18N_H__
#include "gui_private.h"

const char *i18n_get_string(const char *text);
int _i18n_set_parse(GuiConfig *config, const char *language);
int i18n_set_language(const char *language);
GuiI18n *i18n_add_language(const char *language);
void i18n_add_tran(GuiI18n *i18n, const char *english, const char *text);

#endif

