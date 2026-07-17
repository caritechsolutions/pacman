/*
 * =====================================================================================
 *
 *       Filename:  app_pop.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年08月18日 15时18分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __APP_POP_H__
#define __APP_POP_H__
#include "gxcore.h"
#include "gxbook.h"
#include "gui_event.h"

/******************** POP DIALOG ********************/

typedef enum
{
    POP_VAL_OK,
    POP_VAL_CANCEL,
    POP_VAL_NONE,
}PopDlgRet;

typedef enum
{
    POP_TYPE_NO_BTN,
    POP_TYPE_OK,
    POP_TYPE_YES_NO,    /* OK Cancel */
    POP_TYPE_WAIT,
    POP_TYPE_NO_EXIT,
}PopDlgType;

typedef enum
{
    POP_MODE_BLOCK = 0,
    POP_MODE_UNBLOCK
}PopDlgMode;

typedef enum
{
    POP_FORMAT_POPUP = 0,
    POP_FORMAT_DLG
}PopDlgFormat;

typedef enum
{
    POP_DISPLAY_ON_BOT = 0,
    POP_DISPLAY_ON_TOP,
}PopDlgDisplyType;

typedef struct
{
    uint16_t x;
    uint16_t y;
}PopDlgPos;

typedef int (*CreateCb)(void);
typedef int (*ExitCb)(PopDlgRet);
typedef struct
{
    PopDlgType          type;
    PopDlgMode         mode;
    PopDlgFormat    format;
    char*               title;
    char*               str;
    PopDlgPos           pos;        /* pos.x = 0, used hcentre & vcentre */
    CreateCb create_cb;
    ExitCb      exit_cb;
    uint32_t timeout_sec;
    PopDlgRet default_ret;
    bool show_time;
}PopDlg;

typedef struct
{
    PopDlgType          type;
    PopDlgFormat        format;
    char*               title;
    char*               str;
    char*               font_style;
    char*               font_default;
    PopDlgPos           pos;        /* pos.x = 0, used hcentre & vcentre */
    CreateCb            create_cb;
    ExitCb              exit_cb;
    uint32_t            timeout_sec;
    PopDlgRet           default_ret;
    bool                show_time;
}PopDlgEx;

typedef enum
{
    PASSWD_OK,
    PASSWD_ERROR,
    PASSWD_CANCEL
}PopPwdResult;

typedef struct
{
    PopPwdResult result;
    GUI_Event *event;
}PopPwdRet;
/******************** POP INPUT ********************/
typedef struct pop_keyboard PopKeyboard;
struct pop_keyboard
{
    PopDlgPos pos;
    char *in_name;
    char *out_name;
    uint32_t max_num;
    PopDlgRet in_ret;
    int lang_sel; // enum TIME_SET_LANG_NAME    //sattv ftaepg add , 20240701
    void (*change_cb)(PopKeyboard*);
    void (*release_cb)(PopKeyboard*);
    void *usr_data;
};

/******************** POP BOOK********************/
typedef enum
{
	EXEC_WND_NONE,
	EXEC_WND_FULLSCREEN,
	EXEC_WND_MENU,
}_BookExecWnd;

typedef struct
{
    GxBook book;
    _BookExecWnd book_wnd;
}book_cache;

typedef struct
{
    book_cache cache;
    PopDlgRet ret;
}BookDlgRet;

typedef int (*BookExitCb)(BookDlgRet *book_ret);
typedef struct
{
    GxBookType type;
    PopDlgFormat format;
    book_cache cache;
    PopDlgPos  pos;        /* pos.x = 0, used hcentre & vcentre */
    BookExitCb exit_cb;
    uint32_t timeout_sec;
}BookDlg;

/******************** POP PASSWD********************/
typedef enum
{
    KEY_MSG_KEEPON = 0,
    KEY_MSG_STOP
}PasswdKeyMsgState;
typedef int (*PasswdExitCB)(PopPwdRet *ret, void *usr_data);
typedef struct
{
    PopDlgPos pos;
    PasswdExitCB passwd_exit_cb;
    PasswdKeyMsgState passwd_key_msg;
    char *str;
    void *usr_data;
}PasswdDlg;

typedef int (*passwd_dlg_operate_cb)(void *usr_data);

/******************** POP List********************/
typedef enum {
    POP_MULTI_SELECT_DISABLE = 0,
    POP_MULTI_SELECT_ENABLE = 0xaf1234bf
}PopMutiSelectEnable_e;
typedef int (*PopListExitCb)(int32_t ret_sel, unsigned short key);
typedef struct
{
    PopDlgPos pos;
    PopDlgMode mode;
    PopListExitCb exit_cb;
    char *title;
    int item_num;
    char **item_content;
    int sel;
    bool show_num;
    PopMutiSelectEnable_e muti_select;
    char *muti_select_arry;
}PopList;

/******************** EXTERN FUNCTION ********************/
/*
attention : 2023-08-01
   外部调用popdlg_create(),poplist_create()的时候，请把mode = POP_MODE_UNBLOCK
   因为POP_MODE_BLOCK会把消息阻塞。目前POP_MODE_BLOCK的代码还保存着。
*/

extern PopDlgRet popdlg_create(PopDlg* dlg);
extern void popdlg_destroy(void);
extern status_t multi_language_keyboard_create(PopKeyboard *keyboard);
extern status_t passwd_dlg_create(PasswdDlg *passwd_dlg);
extern int app_passwd_dlg_operate(passwd_dlg_operate_cb cb);

extern int poplist_create(PopList* pop_list);
/*
add : 2023.08.10
poplist_create()的参数mode = UNBLOCK，外部调用的时候，会申请内存数据然后赋值给item_content，
等poplis结束的时候，让外部去释放内存，需要额外保存内存指针。所以借用poplist实现里有个全局变量保存着这个参数，
所以可以删除相关的内存数据。所以外部在exit_cb里调用poplist_destory_cb_free_item_content来实现这个。
*/
extern int poplist_destory_cb_free_item_content(void);

extern void app_pop_sort_create(PopDlgPos pop_pos, uint32_t prog_pos, void (*exit_cb)(PopDlgRet,uint32_t));
extern void app_ch_pid_pop(PopDlgPos pop_pos, uint32_t prog_id, void (*exit_cb)(PopDlgRet,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t));
extern void app_update_pop_dlg(const char* window_name);
extern void app_pop_reset_timeout(int timeout);
extern void app_pop_set_string(const char* string);
extern void app_pop_set_exitcb(int (*Exit_Cb)(PopDlgRet));
extern PopDlgDisplyType app_get_popdlg_display_type(void);
extern void app_set_popdlg_display_type(PopDlgDisplyType type);
#endif
