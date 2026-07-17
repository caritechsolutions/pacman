#ifndef APP_PROG_NETUP_H
#define APP_PROG_NETUP_H

#include "app_config.h"
typedef enum
{
    PROG_UP_START = 0,
    PROG_UP_STOP,
    PROG_UP_FAIL,
    PROG_UP_SUCC,
}ProgupStateType;
typedef struct _AppMsgProgupStateUiCotrol
{
    ProgupStateType type;
    int time;
    char* str;
}AppMsgProgupStateUiCotrol;
void app_prog_netup_menu_create(void);
bool app_prog_netup_state_get(void);
int app_prog_netup_msg_proc(GxMessage *msg);
#endif

