#include "gui_key.h"
#include "SDL_keysym.h"
#include "gui_key_test.h"
#include "gui_private.h"
#include <gxcore.h>
#include <sys/time.h>
#include <sys/select.h>

#define REGENTRY(REG_NAME) {#REG_NAME, REG_NAME}//, #REG_NAME},
static key_struct KeyMaps[] = {
	REGENTRY(GUIK_ANY),
	REGENTRY(GUIK_NULL),
	REGENTRY(GUIK_FIRST),

	REGENTRY(GUIK_ESCAPE),
	REGENTRY(GUIK_F1),
	REGENTRY(GUIK_F2),
	REGENTRY(GUIK_F3),
	REGENTRY(GUIK_F4),
	REGENTRY(GUIK_F5),
	REGENTRY(GUIK_F6),
	REGENTRY(GUIK_F7),
	REGENTRY(GUIK_F8),
	REGENTRY(GUIK_F9),
	REGENTRY(GUIK_F10),
	REGENTRY(GUIK_F11),
	REGENTRY(GUIK_F12),
	REGENTRY(GUIK_PRINT),
	REGENTRY(GUIK_SCROLLOCK),
	REGENTRY(GUIK_PAUSE),

	REGENTRY(GUIK_BACKQUOTE),
	REGENTRY(GUIK_1),
	REGENTRY(GUIK_2),
	REGENTRY(GUIK_3),
	REGENTRY(GUIK_4),
	REGENTRY(GUIK_5),
	REGENTRY(GUIK_6),
	REGENTRY(GUIK_7),
	REGENTRY(GUIK_8),
	REGENTRY(GUIK_9),
	REGENTRY(GUIK_0),
	REGENTRY(GUIK_MINUS),
	REGENTRY(GUIK_PLUS),
	REGENTRY(GUIK_BACKSPACE),
	REGENTRY(GUIK_INSERT),
	REGENTRY(GUIK_HOME),
	REGENTRY(GUIK_PAGE_UP),

	REGENTRY(GUIK_TAB),
	REGENTRY(GUIK_Q),
	REGENTRY(GUIK_W),
	REGENTRY(GUIK_E),
	REGENTRY(GUIK_R),
	REGENTRY(GUIK_T),
	REGENTRY(GUIK_Y),
	REGENTRY(GUIK_U),
	REGENTRY(GUIK_I),
	REGENTRY(GUIK_O),
	REGENTRY(GUIK_P),
	REGENTRY(GUIK_LEFTBRACKET),
	REGENTRY(GUIK_RIGHTBRACKET),
	REGENTRY(GUIK_RETURN),
	REGENTRY(GUIK_DELETE),
	REGENTRY(GUIK_END),
	REGENTRY(GUIK_PAGE_DOWN),

	REGENTRY(GUIK_CAPSLOCK),
	REGENTRY(GUIK_A),
	REGENTRY(GUIK_S),
	REGENTRY(GUIK_D),
	REGENTRY(GUIK_F),
	REGENTRY(GUIK_G),
	REGENTRY(GUIK_H),
	REGENTRY(GUIK_J),
	REGENTRY(GUIK_K),
	REGENTRY(GUIK_L),
	REGENTRY(GUIK_SEMICOLON),
	REGENTRY(GUIK_QUOTEDBL),
	REGENTRY(GUIK_BACKSLASH),

	REGENTRY(GUIK_LSHIFT),
	REGENTRY(GUIK_Z),
	REGENTRY(GUIK_X),
	REGENTRY(GUIK_C),
	REGENTRY(GUIK_V),
	REGENTRY(GUIK_B),
	REGENTRY(GUIK_N),
	REGENTRY(GUIK_M),
	REGENTRY(GUIK_COMMA),
	REGENTRY(GUIK_PERIOD),
	REGENTRY(GUIK_SLASH),
	REGENTRY(GUIK_RSHIFT),
	REGENTRY(GUIK_UP),

	REGENTRY(GUIK_LCTRL),
	REGENTRY(GUIK_LSUPER),
	REGENTRY(GUIK_LALT),
	REGENTRY(GUIK_SPACE),
	REGENTRY(GUIK_RALT),
	REGENTRY(GUIK_RSUPER),
	REGENTRY(GUIK_RCTRL),

	REGENTRY(GUIK_LEFT),
	REGENTRY(GUIK_DOWN),
	REGENTRY(GUIK_RIGHT),

	/*Number Keyborder*/
	REGENTRY(GUIK_NUMLOCK),
	REGENTRY(GUIK_KP0),
	REGENTRY(GUIK_KP1),
	REGENTRY(GUIK_KP2),
	REGENTRY(GUIK_KP3),
	REGENTRY(GUIK_KP4),
	REGENTRY(GUIK_KP5),
	REGENTRY(GUIK_KP6),
	REGENTRY(GUIK_KP7),
	REGENTRY(GUIK_KP8),
	REGENTRY(GUIK_KP9),
	REGENTRY(GUIK_KP_PERIOD),
	REGENTRY(GUIK_KP_DIVIDE),
	REGENTRY(GUIK_KP_MULTIPLY),
	REGENTRY(GUIK_KP_MINUS),
	REGENTRY(GUIK_KP_PLUS),
	REGENTRY(GUIK_KP_ENTER),
	REGENTRY(GUIK_RED),
	REGENTRY(GUIK_YELLOW),
	REGENTRY(GUIK_BLUE),
	REGENTRY(GUIK_GREEN),
};

int find_sys_code(const char *key_name)
{
	int ret_key = -1;
	int i = 0, count;

	if (key_name == NULL)
		return -1;

	count = sizeof(KeyMaps) / sizeof(key_struct);
	for (i = 0; i < count; i++) {
		if (strcasecmp(KeyMaps[i].name, key_name) == 0) {
			ret_key = KeyMaps[i].sys_code;
			break;
		}
	}

	return ret_key;
}

char *GUI_GetNameByKey(int key)
{
	int i = 0, count;
	count = sizeof(KeyMaps) / sizeof(key_struct);
	for (i = 0; i < count; i++) {
		if(KeyMaps[i].sys_code == key)
			return (KeyMaps[i].name);
	}

	return (NULL);
}

//      void qsort(void *base, size_t nmemb, size_t size,
static int key_compare(const void *k1, const void *k2)
{
	struct scancode_map *key1 = (struct scancode_map*)k1;
	struct scancode_map *key2 = (struct scancode_map*)k2;

	return key1->scan_code - key2->scan_code;
}

static int _find_key(struct keys_table *table, unsigned int scan_code)
{
	int low = 0, high = 0, mid = 0;

	high = table->scancode_count - 1;

	if (table->scancode_sort == 0) {
		qsort(table->key_map, table->scancode_count, sizeof(struct scancode_map), key_compare);
		table->scancode_sort = 1;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		if (table->key_map[mid].scan_code < scan_code)
			low = mid + 1;
		else if (table->key_map[mid].scan_code > scan_code)
			high = mid - 1;
		else
			return table->key_map[mid].sys_code;
	}

	return -1;
}

int find_key(unsigned int scan_code)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	int i = 0, count = 0, key = 0;

	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if((config) && (!config->invalid) && (config->real_key_table.key_map)) {
				key = _find_key(&(config->real_key_table), scan_code);
				if(key > 0)
					return (key);
			}
		}
	}
	return _find_key(&gui_core->config.real_key_table, scan_code);
}

int find_virtualkeys(unsigned int scan_code, unsigned short key)
{
	if (scan_code == 0) {
		return key;
	}

	return find_virtualkey(scan_code);
}

int find_virtualkey(unsigned int key)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	int i = 0, count = 0, k = 0;

	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if((config) && (!config->invalid) && (config->real_key_table.key_map)) {
				k = _find_key(&(config->virtual_key_table), key);
				if(k > 0)
					return (k);
			}
		}
	}
	k = _find_key(&gui_core->config.virtual_key_table, key);

	if (k < 0)
	{
		if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
			count = stack_count(gui_core->resource_config_stack);
			for(i = 0; i < count; i++) {
				config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
				if((config) && (!config->invalid) && (config->real_key_table.key_map)) {
					k = _find_key(&(config->real_key_table), key);
					if(k > 0)
						return (k);
				}
			}
		}
		k = _find_key(&gui_core->config.real_key_table, key);
		if(k < 0)
		{
			k = key;
		}
	}

	return k;
}

static int _key_map(struct keys_table *table, const char *key_name, int scan_code)
{
	int ret_key = 0;

	if (NULL == key_name)
		return -1;

	table->scancode_sort = 0;

	ret_key = find_sys_code(key_name);

	if (ret_key > 0) {
		if(table->key_map == NULL) {
		    table->scancode_count_max = 16;
		    table->key_map = GxCore_Malloc(table->scancode_count_max * sizeof(struct scancode_map));
		}
		else if (table->scancode_count >= table->scancode_count_max) {
		    table->scancode_count_max += 16;
		    table->key_map = GxCore_Realloc(table->key_map, table->scancode_count_max * sizeof(struct scancode_map));
		}
		table->key_map[table->scancode_count].sys_code = ret_key;
		table->key_map[table->scancode_count].scan_code = scan_code;
		table->scancode_count++;

		return 0;
	}

	return -1;
}

int key_map(void *config, const char *key_name, int scan_code)
{
	GuiCore *gui_core = GUI_GetCurrent();

	if(config)
		return _key_map(&(((GuiConfig *)config)->real_key_table), key_name, scan_code);
	else
		return _key_map(&gui_core->config.real_key_table, key_name, scan_code);
}

static int _find_scancode_by_virtualkey(struct keys_table *virtual_key_table, int sys_code)
{
	int i = 0;

	if((NULL == virtual_key_table) || (NULL == virtual_key_table->key_map)) {
		return (-1);
	}

	for(i = 0; i < virtual_key_table->scancode_count; i++) {
		if(virtual_key_table->key_map[i].sys_code == sys_code) {
			return (virtual_key_table->key_map[i].scan_code);
		}
	}

	return (-1);
}

int find_scancode_by_virtualkey(int sys_code)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	int i = 0, count = 0, key = 0;

	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if((config) && (!config->invalid) && (config->virtual_key_table.key_map)) {
				key = _find_scancode_by_virtualkey(&(config->virtual_key_table), sys_code);
				if(key > 0)
					return (key);
			}
		}
	}

	return (_find_scancode_by_virtualkey(&(gui_core->config.virtual_key_table), sys_code));
}

int virtualkey_map(void *config, const char *key_name, int scan_code)
{
	GuiCore *gui_core = GUI_GetCurrent();

	int key = _find_key(&((GuiConfig *)config)->real_key_table, scan_code);

	if (key >= 0) {
		if(config)
			return _key_map(&(((GuiConfig *)config)->virtual_key_table), key_name, scan_code);
		else
			return _key_map(&gui_core->config.virtual_key_table, key_name, scan_code);
	} else {
		return -1;
	}
}

int key_data_free(void *config)
{
	if(config) {
		GxCore_Free(((GuiConfig *)config)->virtual_key_table.key_map);
		GxCore_Free(((GuiConfig *)config)->real_key_table.key_map);
	}

	return (0);
}

unsigned int key_read(GUI_KeySource *source)
{
	unsigned int code = 0;
	static int irr_fd = -1;
	static int panel_fd = -1;
	int max_fd = -1;
	fd_set fds;
	struct timeval timeout;

	FD_ZERO(&fds);

	if (irr_fd == -1) {
		irr_fd = GxCore_Open("/dev/gxirr0", "r");
		if(irr_fd < 0)
		{
			gxlogd("[GUI] No found IRR.\n");
			irr_fd = -2;
		}
	}

	if (panel_fd == -1) {
		panel_fd = GxCore_Open("/dev/gxpanel0", "r");
		if(panel_fd < 0) {
			gxlogd("[GUI] No found Panel.\n");
			panel_fd = -2;
		}
	}

	if (irr_fd > 0) {
		FD_SET(irr_fd, &fds);
		if (irr_fd > max_fd)
			max_fd = irr_fd;
	}

	if (panel_fd > 0) {
		FD_SET(panel_fd, &fds);
		if (panel_fd > max_fd)
			max_fd = panel_fd;
	}

	timeout.tv_sec = 0;
	if(gui.config.schedule_time)
	{
		timeout.tv_usec = gui.config.schedule_time * 1000;
	}
	else
	{
		timeout.tv_usec = 60 * 1000;
	}

	if (select(max_fd + 1, &fds, NULL, NULL, &timeout) > 0) {
		if (FD_ISSET(irr_fd, &fds)) {
			GxCore_Read(irr_fd, &code, sizeof(code), 1);
			*source = GUI_KEY_FROM_IRR;
		} else if (FD_ISSET(panel_fd, &fds)) {
			GxCore_Read(panel_fd, &code, sizeof(code), 1);
			*source = GUI_KEY_FROM_PANEL;
		}
	}

	return code;
}

