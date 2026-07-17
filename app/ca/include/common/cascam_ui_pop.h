#ifndef __CASCAM_UI_POP_H__
#define __CASCAM_UI_POP_H__
#include "gxcore.h"
#include "app.h"

typedef struct
{
    uint16_t x;
    uint16_t y;
}CascamPopDlgPos;

typedef enum
{
    CASCAM_PASSWD_OK,
    CASCAM_PASSWD_ERROR,
    CASCAM_PASSWD_CANCEL
}CascamPopPwdResult;

typedef struct
{
    CascamPopPwdResult result;
    GUI_Event *event;
}CascamPopPwdRet;

typedef enum
{
    CASCAM_PASSWD_REBUILD,//rebuild passwd everytime
    CASCAM_PASSWD_NOT_REBUILD,//not rebuild passwd when it is existed
}CascamPasswdCreateMode;

/******************** POP PASSWD********************/
typedef enum
{
    CASCAM_KEY_MSG_KEEPON = 0,
    CASCAM_KEY_MSG_STOP
}CascamPasswdKeyMsgState;
typedef int (*CascamPasswdExitCB)(CascamPopPwdRet *ret, void *usr_data);
typedef bool (*CascamPasswdCheckCB)(char *passwd_data,int passwd_len);
typedef int (*CascamReplaceKeypress)(GuiWidget *widget, void *usrdata);

typedef struct
{
    CascamPopDlgPos pos;
    CascamPasswdExitCB passwd_exit_cb;

	CascamPasswdCheckCB passwd_check_cb;
    CascamPasswdKeyMsgState passwd_key_msg;
    CascamReplaceKeypress passwd_key_cb;
	CascamPasswdCreateMode passwd_create_mode;

    uint32_t input_len;
    bool block_exit;
	uint32_t duration;

    char *str;
    void *usr_data;
}CascamPasswdDlg;

/******************** EXTERN FUNCTION ********************/
extern void app_flexible_passwd_end_dialog(void* usrdata);
extern status_t flexible_passwd_dlg_create(CascamPasswdDlg *passwd_dlg);
#endif
