#ifndef __SIRIUS_FIREWALL_REG_H__
#define __SIRIUS_FIREWALL_REG_H__

#include "firewall.h"

#define FW_GLOBLE_CFG             (REG_BASE_FIREWALL + 0x000)
#define FW_FILTER_CFG0            (REG_BASE_FIREWALL + 0x004)
#define FW_FILTER_CFG1            (REG_BASE_FIREWALL + 0x008)
#define FW_FILTER_CFG2            (REG_BASE_FIREWALL + 0x00C)
#define FW_FILTER_BASE(i)         (REG_BASE_FIREWALL + 0x010 + 0x10*(i))
#define FW_FILTER_TOP(i)          (REG_BASE_FIREWALL + 0x014 + 0x10*(i))
#define FW_FILTER_RD_MASK(i)      (REG_BASE_FIREWALL + 0x018 + 0x10*(i))
#define FW_FILTER_WR_MASK(i)      (REG_BASE_FIREWALL + 0x01C + 0x10*(i))
#define FW_DEFAULT_RD_MASK        (REG_BASE_FIREWALL + 0x200)
#define FW_DEFAULT_WR_MASK        (REG_BASE_FIREWALL + 0x204)
#define FW_CONFIG_UPLOAD          (REG_BASE_FIREWALL + 0x300)

#define FILTER_MASK                   (8)
#define FW_FILTER_SKIP                (FW_FILTER01_BASE - FW_FILTER00_BASE)

#endif
