#ifndef __APP_COMMON_SEARCH_VIEW_H__
#define __APP_COMMON_SEARCH_VIEW_H__
#include <gxtype.h>
#include "app_pop.h"

typedef int (*CSTSignalCb)(void);

typedef enum
{
    CST_ITEM_POP_CHOICE = 0,
    CST_ITEM_CHOICE,
    CST_ITEM_EDIT,
    CST_ITEM_PUSH
}CSTItemType;

typedef enum
{
    CST_ITEM_NORMAL = 0,
    CST_ITEM_HIDE,
    CST_ITEM_DISABLE
}CSTItemStatus;


typedef struct
{
    char *string;
    char *maxlen;
    char *format;
    char *intaglio;
    char *default_intaglio;
}CSTEditProperty;

typedef struct
{
    char *content;
    int  sel;
}CSTCmbProperty;

typedef struct
{
    char *string;
    bool roll_format;
}CSTBtnProperty;

typedef union
{
    CSTCmbProperty  itemPropertyCmb;
    CSTEditProperty itemPropertyEdit;
    CSTBtnProperty  itemPropertyBtn;
}CSTItemProperty;

typedef union
{
    int  value;
    char *string;
}CSTItemData;

typedef struct
{
    int (*CmbChange)(int);
    int (*CmbPress)(void);
}CSTCmbCb;

typedef struct
{
    int (*EditReachEnd)(char *);
    int (*EditPress)(unsigned short);
    int (*EditPressEnd)(void);
}CSTEditCb;

typedef struct
{
    int (*BtnPress)(void);
}CSTBtnCb;

typedef union
{
    CSTCmbCb  cmbCb;
    CSTEditCb editCb;
    CSTBtnCb  btnCb;
}CSTItemCb;

typedef enum
{
	CST_EXIT_NORMAL,
	CST_EXIT_ABANDON,
}CSTExitType;

typedef enum
{
    CST_EXIT_RET_DEFAULT,
    CST_EXIT_RET_POP,
}CSTExitRet;

typedef bool (*CSTCheckCb)(void);

typedef struct
{
    char            *itemTitle;
    int             para;
    CSTItemType     itemType;
    CSTItemStatus   itemStatus;
    CSTItemCb       itemCb;
    CSTItemProperty itemProperty;
    CSTCheckCb      checkCb;
}CSTItem;

typedef int (*CSTExitCb)(CSTExitType);
typedef int (*CSTItemChangeCb)(int sel);

typedef struct
{
    CSTSignalCb create_cb;
    CSTExitCb   exit_cb;
	CSTItemChangeCb	item_change;
    CSTSignalCb got_focus;
    CSTSignalCb lost_focus;
}CSTSignal;

typedef struct
{
	char *keyName;
	int (*keyPressFun)(void);
}FuncKey;

typedef struct
{
	FuncKey redKey;
	FuncKey greenKey;
	FuncKey yellowKey;
	FuncKey blueKey;
	FuncKey hideKey;
}CSTFuncKey;

typedef struct
{
    bool        signal_show;
	bool		content_show;
    int         cur_tuner;
    int         itemNum;
    char        *menuTitle;
    char        *titleImage;
	char		*Content;
    CSTItem     *item;
    CSTSignal   signal;
    CSTFuncKey  funckey;
}CSTOpt;

extern int32_t app_cst_dialog_create(CSTOpt *opt);
extern int32_t app_cst_choice_item_sel_set(uint32_t sel, int32_t value);
extern int32_t app_cst_item_data_update(uint32_t sel);
extern int32_t app_cst_content_data_update(void);
extern int32_t app_cst_item_data_get(uint32_t sel, CSTItemData *data);
extern bool app_cst_callback_handler(CheckCb check_cb, ExitCb exit_cb);
extern void app_cst_key_data_updata(void);
#endif
