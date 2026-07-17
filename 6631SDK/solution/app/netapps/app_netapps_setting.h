#ifndef __APP_NETAPPS_SETTING_H__
#define __APP_NETAPPS_SETTING_H__

typedef enum
{
	SET_ITEM_CHOICE = 0,
	SET_ITEM_EDIT,
	SET_ITEM_PUSH
}SetItemType;

typedef enum
{
	SET_ITEM_NORMAL = 0,
	SET_ITEM_HIDE,
	SET_ITEM_DISABLE,
}SetItemStatus;

typedef enum
{
	SET_EDIT_STRING = 0,
	SET_EDIT_DIGIT,
	SET_EDIT_IP,
	SET_EDIT_TIME,
	SET_EDIT_DATE,
	SET_EDIT_FLOAT,
}SetEditFormat;

typedef enum
{
	SET_ETYPE_NORMAL = 0,
	SET_ETYPE_ABANDON,
}SetExitType;

typedef enum
{
	SET_ERET_NORMAL = 0,
	SET_ERET_ABANDON,
}SetExitRet;

typedef enum
{
	SET_ON = 0,
	SET_OFF,
}SetStatus;

typedef int(*SetExitCb)(SetExitType);
typedef int(*SetChoiceChange)(int);
typedef int(*SetChoicePress)(int);
typedef int(*SetEditEnd)(char *);
typedef int(*SetEditPress)(unsigned short);
typedef int(*SetBtnPress)(unsigned short);
typedef int (*SetKeyFun)(void);

unsigned int app_netapps_setting_malloc(int item_num);

void app_netapps_setting_free(unsigned int handle);

void app_netapps_setting_set_title(unsigned int handle, char *title);

void app_netapps_setting_set_logo(unsigned int handle, char *logo);

void app_netapps_setting_set_exit_cb(unsigned int handle, SetExitCb cb);

void app_netapps_setting_set_item_title(unsigned int handle, int index, char *title);

void app_netapps_setting_set_item_type(unsigned int handle, int index, SetItemType type);

void app_netapps_setting_set_item_status(unsigned int handle, int index, SetItemStatus status);

void app_netapps_setting_set_item_content(unsigned int handle, int index, char *content);

void app_netapps_setting_set_choice_sel(unsigned int handle, int index, int sel);

int app_netapps_setting_get_choice_sel(unsigned int handle, int index);

void app_netapps_setting_set_edit_maxlen(unsigned int handle, int index, char *maxlen);

void app_netapps_setting_set_edit_format(unsigned int handle, int index, SetEditFormat format);

void app_netapps_setting_set_edit_intaglio(unsigned int handle, int index, char *intaglio);

void app_netapps_setting_set_edit_default_intaglio(unsigned int handle, int index, char *default_intaglio);

char *app_netapps_setting_get_edit_content(unsigned int handle, int index);

void app_netapps_setting_set_button_roll(unsigned int handle, int index, int onoff);

void app_netapps_setting_set_choice_change_cb(unsigned int handle, int index, SetChoiceChange cb);

void app_netapps_setting_set_choice_press_cb(unsigned int handle, int index, SetChoicePress cb);

void app_netapps_setting_set_edit_reachend_cb(unsigned int handle, int index, SetEditEnd cb);

void app_netapps_setting_set_edit_press_cb(unsigned int handle, int index, SetEditPress cb);

void app_netapps_setting_set_button_press_cb(unsigned int handle, int index, SetBtnPress cb);

int app_netapps_setting_create(unsigned int handle);

void app_netapps_setting_destroy(unsigned int handle, SetExitType type);

void app_netapps_setting_close(unsigned int handle);

void app_netapps_setting_sync_item_data(unsigned int handle, int index);

void app_netapps_setting_update_item_state(unsigned int handle, int index, SetItemStatus status);

void app_netapps_setting_update_item_content(unsigned int handle, int index, char *content);

void app_netapps_setting_update_choice_sel(unsigned int handle, int index, int sel);

void app_netapps_setting_set_red_key(unsigned int handle, char *name, SetKeyFun func);

void app_netapps_setting_set_green_key(unsigned int handle, char *name, SetKeyFun func);

void app_netapps_setting_set_blue_key(unsigned int handle, char *name, SetKeyFun func);

void app_netapps_setting_set_yellow_key(unsigned int handle, char *name, SetKeyFun func);

void app_netapps_setting_exit_key_set(unsigned int handle, SetStatus status);

void app_netapps_setting_top_tip_set(unsigned int handle, SetStatus status);

void app_netapps_setting_top_tipimg_set(unsigned int handle, SetStatus status);

void app_netapps_setting_buttom_tip_set(unsigned int handle, SetStatus status);

void app_netapps_setting_time_set(unsigned int handle, SetStatus status);

void app_netapps_setting_show_msg(unsigned int handle, char *msg);

#endif
