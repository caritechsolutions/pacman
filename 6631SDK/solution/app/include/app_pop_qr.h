#include "gui_core.h"
#ifndef __APP_POP_QR_H__
#define __APP_POP_QR_H__

typedef struct
{
    uint16_t x;
    uint16_t y;
}PopQrPos;

typedef void (*QrExitCb)(void);

typedef struct
{
	char *str_title;
	char *str_content;
	PopQrPos pos;
	QrExitCb exit_cb;
	int forbidExitKey;
}PopQrParas;

void app_pop_create_qr(PopQrParas *paras);
void app_pop_end_qr(void);

#endif

