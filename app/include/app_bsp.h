/*
 * =====================================================================================
 *
 *       Filename:  app_bsp_handle.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2010年10月18日 01时07分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_BSP_H__
#define __APP_BSP_H__

#include <sys/ioctl.h>
#include "bsp_dev_panel.h"
#include "bsp_dev_gpio.h"
#include "app.h"
#include "tree.h"
#include "parser.h"
//#include "bsp_dev_reg.h"


typedef struct {
    char *gname;
    char *iname;
}BspItem;

extern status_t bsp_parser(const char *filename, BspItem *item, char *buf, size_t size, int mode);
extern status_t app_get_bsp_key_list(const char *filename, char **key_name, BspItem *item, uint32_t *keyvalue, int size);


#endif

