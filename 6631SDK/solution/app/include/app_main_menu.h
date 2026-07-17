#ifndef __APP_MAIN_MENU_H__
#define __APP_MAIN_MENU_H__

#include "app_str_id.h"

typedef void (*CreateFunc)(void);
typedef int (*CheckFunc)(void);
typedef int (*KeypressFunc)(int);
typedef char* (*ContentFunc)(void);

typedef enum {
	MENU_NONE_ATTR = 0,
	MENU_LOCK_FORCE = 1 << 0,
	MENU_LOCK_CHECK = 1 << 1,
	MENU_NETWORK_CHECK = 1 << 2
}MenuAttr;

typedef struct
{
	unsigned int attr;
	CreateFunc create;
	CheckFunc check;
	int appui_id;
}ItemEntry;

typedef struct
{
	KeypressFunc keypress;
	ContentFunc content;
	CheckFunc check;
	int appui_id;
}ItemSetting;

typedef struct
{
	int type;
	union
	{
		ItemEntry item_e;
		ItemSetting item_s;
	}u;
}MainMenuSetting;

typedef struct
{
	bool default_last_sub;
	bool item_move;
	int default_sub;
}MainMenuStyle;

typedef struct
{
	const char *focus_widget;
	int next_ladder_key[5];
	int back_ladder_key[5];
	int next_key_num;
	int back_key_num;
}OperationLadder;

enum
{
	MAIN_MENU_ENTRY,
	MAIN_MENU_SETTING,
};

int app_submenu_enter(unsigned int attr, CreateFunc create);


int mainmenu_item_offset_get(void);
int mainmenu_item_length_get(void);
int mainmenu_item_move_level_get(void);
int mainmenu_item_listview_visual_get(void);
int mainmenu_item_arrow_exist_get(void);
int mainmenu_item_move_step_get(void);
#endif
