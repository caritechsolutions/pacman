#ifndef __FIREWALL_H__
#define  __FIREWALL_H__

#include "io.h"
#include "gxhwlib_registers.h"
#include "gx_firewall.h"

void sirius_firewall_init(void);
int sirius_firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag);

void canopus_firewall_init(void);
int canopus_firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag);

#endif
