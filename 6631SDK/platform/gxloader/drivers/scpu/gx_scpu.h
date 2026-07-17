#ifndef __GXSCPU_H__
#define __GXSCPU_H__

#include <gxhwlib_registers.h>

#define GXSCPU_CONFIG_BASE_ADDR     REG_BASE_SCPU

#define GXSCPU_AP_CMD               (GXSCPU_CONFIG_BASE_ADDR + 0x0)
#define GXSCPU_AP_CMD_EN            (GXSCPU_CONFIG_BASE_ADDR + 0x4)
#define GXSCPU_SP_STATUS            (GXSCPU_CONFIG_BASE_ADDR + 0X8)
#define GXSCPU_SP_STATUS_EN         (GXSCPU_CONFIG_BASE_ADDR + 0x0c)
#define GXSCPU_COSE_BASE            (GXSCPU_CONFIG_BASE_ADDR + 0x10)
#define GXSCPU_PARAM_BASE           (GXSCPU_CONFIG_BASE_ADDR + 0x14)
#define GXSCPU_PUB_ID_LO            (GXSCPU_CONFIG_BASE_ADDR + 0x18)
#define GXSCPU_PUB_ID_HI            (GXSCPU_CONFIG_BASE_ADDR + 0x1c)
#define GXSCPU_SKETCH0              (GXSCPU_CONFIG_BASE_ADDR + 0x20)
#define GXSCPU_SKETCH2              (GXSCPU_CONFIG_BASE_ADDR + 0x24)
#define GXSCPU_CW_WEN               (GXSCPU_CONFIG_BASE_ADDR + 0x40)
#define GXSCPU_CW_ADDR              (GXSCPU_CONFIG_BASE_ADDR + 0x44)
#define GXSCPU_CW_DATA_LO           (GXSCPU_CONFIG_BASE_ADDR + 0x48)
#define GXSCPU_CW_DAT_HI            (GXSCPU_CONFIG_BASE_ADDR + 0x4c)
#define GXSCPU_RETIRE_PC            (GXSCPU_CONFIG_BASE_ADDR + 0x50)
#define GXSCPU_PROT_SIZE            (GXSCPU_CONFIG_BASE_ADDR + 0x54)
#define GXSCPU_CTIM_CALIB           (GXSCPU_CONFIG_BASE_ADDR + 0x58)
#define GXSCPU_DSK_0                (GXSCPU_CONFIG_BASE_ADDR + 0x60)
#define GXSCPU_DSK_1                (GXSCPU_CONFIG_BASE_ADDR + 0x64)
#define GXSCPU_DSK_2                (GXSCPU_CONFIG_BASE_ADDR + 0x68)
#define GXSCPU_DSK_3                (GXSCPU_CONFIG_BASE_ADDR + 0x6c)
#define GXSCPU_DSK_4                (GXSCPU_CONFIG_BASE_ADDR + 0x70)
#define GXSCPU_DSK_5                (GXSCPU_CONFIG_BASE_ADDR + 0x74)

#endif
