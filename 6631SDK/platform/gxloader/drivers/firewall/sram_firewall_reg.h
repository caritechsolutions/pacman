#ifndef __SRAM_FIREWALL_REG_H__
#define __SRAM_FIREWALL_REG_H__

#include "firewall.h"

#define REG_SFW_BOTTOM_SIZE (REG_BASE_FIREWALL_SRAM + 0x8)
#define REG_SFW_AUTH        (REG_BASE_FIREWALL_SRAM + 0xC)

#define BIT_SRAM_BOTTOM_RX  (0)
#define BIT_SRAM_BOTTOM_TX  (1)
#define BIT_SRAM_TOP_RX     (2)
#define BIT_SRAM_TOP_TX     (3)

#endif
