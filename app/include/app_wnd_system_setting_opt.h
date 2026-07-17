#ifndef SYSTEM_SETTING_H
#define SYSTEM_SETTING_H
#include "app_windows.h"

typedef enum
{
	GIF_HIDE,
	GIF_SHOW,
}GifDisplayStatus;

typedef enum
{
	TIP_SHOW,
	TIP_HIDE,
}TipDisplayStatus;

typedef enum
{
	ITEM_CHOICE,
	ITEM_POP_CHOICE,
	ITEM_EDIT,
	ITEM_PUSH
}ItemSettingType;

typedef enum
{
	ITEM_NORMAL,
	ITEM_HIDE,
	ITEM_DISABLE
}ItemDisplayStatus;

typedef struct
{
	char *string;
	char *maxlen;
	char *format;
	char *intaglio;
	char *default_intaglio;
}EditProperty;

typedef struct
{
	char *content;
	int sel;
}CmbProperty;

typedef struct
{
	char *string;
    bool roll_format;
}BtnProperty;

typedef union
{
	CmbProperty itemPropertyCmb;
	EditProperty itemPropertyEdit;
	BtnProperty itemPropertyBtn;
}ItemProPerty;

typedef struct
{
	int (*CmbChange)(int);
	int (*CmbPress)(int);
}CmbCallback;

typedef struct
{
	int (*EditReachEnd)(char *);
	int (*EditPress)(unsigned short);
}EditCallback;

typedef struct
{
	int (*BtnPress)(unsigned short);
}BtnCallback;

typedef union
{
	CmbCallback cmbCallback;
	EditCallback editCallback;
	BtnCallback btnCallback;
}ItemCallback;

typedef struct
{
	char *keyName;
	int (*keyPressFun)(void);
}FuncKey;

typedef struct
{
	FuncKey redKey;
	FuncKey greenKey;
	FuncKey blueKey;
	FuncKey yellowKey;
}MenuFuncKey;

typedef struct
{
	char *itemTitle;
	ItemSettingType itemType;
	ItemProPerty itemProperty;
	ItemCallback itemCallback;
	ItemDisplayStatus itemStatus;
}SystemSettingItem;

typedef enum
{
	BG_MENU = 0,
	BG_PLAY_RPOG,
}BackGroundMode;

typedef enum
{
	EXIT_NORMAL,
	EXIT_ABANDON,
}ExitType;

typedef enum
{
    EXIT_RET_DEFAULT,
    EXIT_RET_BLOCK,
    EXIT_RET_POP,
}ExitRet;

typedef struct
{
	char *menuTitle;
	int itemNum;
	BackGroundMode backGround;
	char *titleImage;
	SystemSettingItem *item;
	MenuFuncKey menuFuncKey;
	int (*exit)(ExitType);
	bool forbidExitKey;
	TipDisplayStatus topTipDisplay;
    TipDisplayStatus topTipImgDisplay;
	TipDisplayStatus bottmTipDisplay;
	TipDisplayStatus timeDisplay;
	GifDisplayStatus gifDisplay;
}SystemSettingOpt;

int app_system_get_item_total_num(void);
int app_system_get_sel(void);
char *app_system_get_title(void);
char *app_system_get_combobox(uint32_t index);
char *app_system_get_edit(uint32_t index);
char *app_system_get_button(uint32_t index);
char *app_system_get_item_title(uint32_t index);
char *app_system_get_item_value(uint32_t index);
WndStatus app_system_set_create(SystemSettingOpt *settingOpt);
void app_system_set_destroy(ExitType exit_type);
WndStatus app_system_set_block_create(SystemSettingOpt *settingOpt); //gui 阻塞方式，适合需要返回值的情况

//zl
void app_system_setting_gif_show(void);
void app_system_setting_gif_hide(void);

void app_system_set_item_property(int index, ItemProPerty *property);
void app_system_set_item_state_update(int index, ItemDisplayStatus state);
void app_system_set_item_data_update(int index);
void app_system_set_focus_item(int index);
void app_system_set_unfocus_item(int index);
void app_system_set_wnd_update(void);
void app_system_set_item_title(int sel, char *new_title);
void app_system_set_item_event(int sel, GUI_Event *event);
void app_system_set_wnd_event(GUI_Event *event);
void app_system_reset_unfocus_image(void);
#endif
