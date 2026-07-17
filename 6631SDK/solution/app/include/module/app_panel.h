/*
 * =====================================================================================
 *
 *       Filename:  app_panel.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年05月16日 10时33分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __APP_PANEL_H__
#define __APP_PANEL_H__

#include "gxcore.h"
#include "app_bsp.h"
#include "module/app_oem.h"
typedef enum
{
	PANEL_BOARD_TYPE,
	PANEL_LED_CONVERSE,
	PANEL_LED_LOCK,
	PANEL_LED_VALUE,
	PANEL_KEY_VALUE,
	PANEL_LED_STANDBY,
	PANEL_DIGTAL_VALUE,
	PANEL_LONGKEY_VALUE,
    PANEL_TYPE_MAX
}app_panel_type;



extern status_t app_oem_get(app_oem_type type, char * buf, size_t size);
extern status_t app_oem_get_int(app_oem_type type, int32_t *result);
extern int app_bsp_panel_init(void);
extern int app_bsp_panel_close(void);
extern int app_panel_ioctl_handle(unsigned int cmd, void *value);
extern uint32_t app_get_panel_keycode(PanelKeymap key_index);
extern int app_get_keymap_data(const char *key_name, unsigned int **key_val);
extern uint32_t app_get_panel_key(void);
extern status_t app_panel_get_int(app_panel_type type, int32_t *result);
#endif

