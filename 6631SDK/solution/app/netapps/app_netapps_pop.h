#ifndef __APP_NETAPPS_POP_H__
#define __APP_NETAPPS_POP_H__

typedef enum
{
    POP_RET_OK = 0,
    POP_RET_CANCEL,
}PopRetType;

typedef enum
{
	PDLG_BTN_YES_NO = 0,
	PDLG_BTN_OK,
	PDLG_BTN_NONE,
}PdlgBtnType;

typedef enum
{
	PW_RET_OK = 0,
	PW_RET_ERROR,
	PW_RET_CANCEL,
}PwRetType;

typedef int (*PopCreateCb)(void);
typedef int (*PopExitCb)(PopRetType);
typedef int (*PoplistExitCb)(PopRetType, int);
typedef int (*PwExitCb)(PwRetType, void *);
typedef int (*PqrExitCb)(void);
typedef void (*KbChangeCb)(char*);
typedef void (*KbReleaseCb)(PopRetType, char*);

unsigned int app_netapps_popdlg_malloc(void);

void app_netapps_popdlg_free(unsigned int handle);

void app_netapps_popdlg_set_button_type(unsigned int handle, PdlgBtnType type);

void app_netapps_popdlg_set_title(unsigned int handle, char *title);

void app_netapps_popdlg_set_content(unsigned int handle, char *content);

void app_netapps_popdlg_set_position(unsigned int handle, int pos_x, int pos_y);

void app_netapps_popdlg_set_timeout(unsigned int handle, int sec);

void app_netapps_popdlg_set_default_ret(unsigned int handle, PopRetType ret);

void app_netapps_popdlg_set_time_display(unsigned int handle, int enable);

void app_netapps_popdlg_set_create_cb(unsigned int handle, PopCreateCb cb);

void app_netapps_popdlg_set_exit_cb(unsigned int handle, PopExitCb cb);

PopRetType app_netapps_popdlg_create(unsigned int handle);

void app_netapps_popdlg_destroy(unsigned int handle);


unsigned int app_netapps_keyboard_malloc(int max_num);

void app_netapps_keyboard_free(unsigned int handle);

void app_netapps_keyboard_set_instr(unsigned int handle, char *instr);

void app_netapps_keyboard_set_position(unsigned int handle, int pos_x, int pos_y);

void app_netapps_keyboard_set_change_cb(unsigned int handle, KbChangeCb cb);

void app_netapps_keyboard_set_release_cb(unsigned int handle, KbReleaseCb cb);

void app_netapps_keyboard_disable_chars(unsigned int handle, char *chars);

PopRetType app_netapps_keyboard_create(unsigned int handle);

void app_netapps_keyboard_destroy(unsigned int handle);


unsigned int app_netapps_poplist_malloc(int item_num);

void app_netapps_poplist_free(unsigned int handle);

void app_netapps_poplist_set_title(unsigned int handle, char *title);

void app_netapps_poplist_set_content(unsigned int handle, char **content);

void app_netapps_poplist_set_position(unsigned int handle, int pos_x, int pos_y);

void app_netapps_poplist_set_sel(unsigned int handle, int sel);

void app_netapps_poplist_set_index_display(unsigned int handle, int enable);

void app_netapps_poplist_set_exit_cb(unsigned int handle, PoplistExitCb cb);

PopRetType app_netapps_poplist_create(unsigned int handle);

void app_netapps_poplist_destroy(unsigned int handle);


unsigned int app_netapps_pwdlg_malloc(void);

void app_netapps_pwdlg_free(unsigned int handle);

void app_netapps_pwdlg_set_content(unsigned int handle, char *content);

void app_netapps_pwdlg_set_position(unsigned int handle, int pos_x, int pos_y);

void app_netapps_pwdlg_set_exit_cb(unsigned int handle, PwExitCb cb);

PopRetType app_netapps_pwdlg_create(unsigned int handle);

void app_netapps_pwdlg_destroy(unsigned int handle);


unsigned int app_netapps_popqr_malloc(void);

void app_netapps_popqr_free(unsigned int handle);

void app_netapps_popqr_set_title(unsigned int handle, char *title);

void app_netapps_popqr_set_content(unsigned int handle, char *content);

void app_netapps_popqr_set_position(unsigned int handle, int pos_x, int pos_y);

void app_netapps_popqr_set_exit_cb(unsigned int handle, PqrExitCb cb);

void app_netapps_popqr_exit_key_set(unsigned int handle, int disable);

PopRetType app_netapps_popqr_create(unsigned int handle);

void app_netapps_popqr_destroy(unsigned int handle);


unsigned int app_netapps_poptext_malloc(void);

void app_netapps_poptext_free(unsigned int handle);

void app_netapps_poptext_set_title(unsigned int handle, char *title);

void app_netapps_poptext_set_content(unsigned int handle, char *content);

PopRetType app_netapps_poptext_create(unsigned int handle);

void app_netapps_poptext_destroy(unsigned int handle);

#endif
