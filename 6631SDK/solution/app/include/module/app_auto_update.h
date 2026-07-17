#ifndef APP_AUTO_UPDATE_H
#define APP_AUTO_UPDATE_H

#include "app_config.h"
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
typedef enum
{
    ACU_UPDATE_START = 0,
    ACU_UPDATE_PROG,
    ACU_UPDATE_NIT_START,
    ACU_UPDATE_BAT,
    ACU_UPDATE_PMT,
    ACU_UPDATE_LCN,
    ACU_UPDATE_END
}AcuStateType;
typedef struct _AppMsgAcuStateUiCotrol
{
    AcuStateType type;
    int timeout;
    uint16_t version;
    char* show_str;
}AppMsgAcuStateUiCotrol;
int app_auto_update_msg_proc(GxMessage *msg);
void app_auto_update_send_ui_msg(AcuStateType type,int sec, const char* string);
#if AUTO_UPDATE_SUPPORT
void app_auto_update_start(GxMsgProperty_NodeByPosGet *prog_node);
void app_auto_update_restart(void);
void app_auto_update_stop(void);
uint8_t app_get_auto_update_flag(void);
#endif
#endif
#endif

