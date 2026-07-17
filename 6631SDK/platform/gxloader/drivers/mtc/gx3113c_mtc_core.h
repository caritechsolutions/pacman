/*****************************************************************************
*                          CONFIDENTIAL                             
*        Hangzhou GuoXin Science and Technology Co., Ltd.            
*                      (C)2007, All right reserved
******************************************************************************
* Purpose   :   
* Release History:
  VERSION       Date              AUTHOR         Description
  0.1           2014.10.12                  creation
*****************************************************************************/

#ifndef _GX3113C_MTC_REG_H_20160229_
#define _GX3113C_MTC_REG_H_20160229_

/* Includes */
/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MTC_Regs_3113c_s
{
	unsigned int     MTC_CTRL1   ;          // 0x00
	unsigned int     MTC_KEY1_L  ;          // 0x04
	unsigned int     MTC_KEY1_H  ;          // 0x08
	unsigned int     MTC_KEY2_L  ;          // 0x0c
	unsigned int     MTC_KEY2_H  ;          // 0x10
	unsigned int     MTC_KEY3_L  ;          // 0x14
	unsigned int     MTC_KEY3_H  ;          // 0x18
	unsigned int     MTCR_SDR_START_ADDR ;  // 0x1c
	unsigned int     MTCW_SDR_START_ADDR ;  // 0x20
	unsigned int     MTCR_SDR_DATA_LEN   ;  // 0x24
	unsigned int     MTC_CTRL2   ;          // 0x28
	unsigned int     MTC_KEY4_L  ;          // 0x2c
	unsigned int     MTC_KEY4_H  ;          // 0x30
	unsigned int     MTC_COUNTER[4]  ;      // 0x34,38,3c,40
	unsigned int     MTC_IV1[4]      ;      // 0x44,48,4c,50
	unsigned int     MTC_MULTI_DATA_KEY_H ; // 0x54
	unsigned int     MTC_MULTI_DATA_KEY_L ; // 0x58
	unsigned int     MTC_CA_MODE ;          // 0x5c
	unsigned int     MTC_DCK[6]  ;          // 0x60,64,68,6c,70,74
	unsigned int     MTC_DCW[4]  ;          // 0x78,7c,80,84
	unsigned int     MTC_Reserv_1[2]   ;    // 0x88,8c
	unsigned int     MTC_CA_ADDR   ;        // 0x90
	unsigned int     MTC_Reserv_2[2]   ;    // 0x94,98
	unsigned int     MTC_NDS_CONTROL_SET ;  // 0x9c
	unsigned int     MTC_NDS_KEY_GEN ;      // 0xa0
	unsigned int     MTC_NDS_KEY_WRITE_ADDR;// 0xa4
	unsigned int     MTC_NDS_KEY_FULL ;     // 0xa8
	unsigned int     MTC_Reserv_3[21]   ;   // 0xac, 0xb0,0xc0,0xd0,0xe0,0xf0
	unsigned int     MTC_DSK[6] ;           // 0x100,....0x110,0x114 
	unsigned int     MTC_Reserv_4[2]   ;    // 0x118,0x11c
	unsigned int     MTC_CW_SELECT     ;    // 0x120
	unsigned int     MTC_NONCE[4]      ;    // 0x124,128,12c,130
	unsigned int     MTC_DA[4]      ;       // 0x134,138,13c,140
	unsigned int     MTC_Reserv_5[3]   ;    // 0x144,148,14c
	unsigned int     MTC_CA_K3_GEN     ;    // 0x150
	unsigned int     MTC_Reserv_6[3]   ;    // 0x154,158,15c
	unsigned int     MTC_INT_EN        ;    // 0x160
	unsigned int     MTC_INT           ;    // 0x164
	unsigned int     MTC_Reserv_7[6]   ;    // 0x168,16c,170,174,178,17c
	unsigned int     MTC_IV2[4]      ;      // 0x180,184,188,18c
} MTC_Regs_3113c_t;

#define KEY_SELECT_KEY_IPBUS_SET                    0

#define MTC_SET_CA_MODE(rp)                    ((rp)->MTC_CA_MODE |= (1 << 0))

#define MTC_SET_ENCRYPT(rp)                    ((rp)->MTC_CTRL1 |= (1 << 1))
#define MTC_SET_DECRYPT(rp)                    ((rp)->MTC_CTRL1 &= ~(1 << 1))
#define MTC_SET_ALGO_DES(rp)                   ((rp)->MTC_CTRL1 &= ~(1 << 0))
#define MTC_SET_ALGO_3DES(rp)                  ((rp)->MTC_CTRL1 |= (1 << 0))
#define MTC_SET_ALGO_AES128(rp)                ((rp)->MTC_CTRL1 |= ((0 << 3) | (1 << 5)))
#define MTC_SET_ALGO_AES192(rp)                ((rp)->MTC_CTRL1 |= ((1 << 3) | (1 << 5)))
#define MTC_SET_ALGO_AES256(rp)                ((rp)->MTC_CTRL1 |= ((2 << 3) | (1 << 5)))
#define MTC_SET_ALGO_MULTI2(rp)                ((rp)->MTC_CTRL1 |= (2 << 5))
#define MTC_SET_ALGO_SMS4(rp)                  ((rp)->MTC_CTRL1 |= (3 << 5))
#define MTC_SET_SUB_MODE(rp, value)            {(rp)->MTC_CTRL1 &= ~(7 << 7); (rp)->MTC_CTRL1 |= (value << 7);}
#define MTC_SET_SHORT_MSG(rp, value)           ((rp)->MTC_CTRL1 |= ((value & 0x3) << 23))
#define MTC_SET_RESIDUE_MSG(rp, value)         ((rp)->MTC_CTRL1 |= ((value & 0x3) << 21))
#define MTC_SET_KEY_SELECT(rp, value)          {(rp)->MTC_CTRL1 &= ~(7 << 17); (rp)->MTC_CTRL1 |= (value << 17);}

#define MTC_SET_KEY_READY(rp)                  ((rp)->MTC_CTRL1 |= (1 << 2))

#define MTC_SET_CA_ADDR_ODD(rp, id)           ((rp)->MTC_CA_ADDR = ((0xa0 + id * 2) << 16) + (0xa1 + id * 2))
#define MTC_SET_CA_ADDR_EVEN(rp, id)          ((rp)->MTC_CA_ADDR = ((0xa1 + id * 2) << 16) + (0xa0 + id * 2))
#define MTC_ALL_INT_STATUS_GET(rp)             ((rp)->MTC_INT)

#define MTC_SET_COUNTER_31_0(rp, value)        ((rp)->MTC_COUNTER[3] = value)
#define MTC_SET_COUNTER_63_32(rp, value)       ((rp)->MTC_COUNTER[2] = value)
#define MTC_SET_COUNTER_95_64(rp, value)       ((rp)->MTC_COUNTER[1] = value)
#define MTC_SET_COUNTER_127_96(rp, value)      ((rp)->MTC_COUNTER[0] = value)

#define MTC_SET_KEY_31_0(rp, value)            ((rp)->MTC_KEY1_L = value)
#define MTC_SET_KEY_63_32(rp, value)           ((rp)->MTC_KEY1_H = value)
#define MTC_SET_KEY_95_64(rp, value)           ((rp)->MTC_KEY2_L = value)
#define MTC_SET_KEY_127_96(rp, value)          ((rp)->MTC_KEY2_H = value)
#define MTC_SET_KEY_159_128(rp, value)         ((rp)->MTC_KEY3_L = value)
#define MTC_SET_KEY_191_160(rp, value)         ((rp)->MTC_KEY3_H = value)
#define MTC_SET_KEY_223_192(rp, value)         ((rp)->MTC_KEY4_L = value)
#define MTC_SET_KEY_255_224(rp, value)         ((rp)->MTC_KEY4_H = value)

#define MTC_SET_READ_BUF_ADDR(rp, value)       ((rp)->MTCR_SDR_START_ADDR = value & 0x7fffffff)
#define MTC_SET_WRITE_BUF_ADDR(rp, value)      ((rp)->MTCW_SDR_START_ADDR = value & 0x7fffffff)
#define MTC_SET_READ_BUF_LEN(rp, value)        ((rp)->MTCR_SDR_DATA_LEN = value & 0x1fffff)
#define MTC_MAX_BUF_LEN                        ((unsigned int)0x1fffff)

#define MTC_SET_DES_READY(rp)                  ((rp)->MTC_CTRL2 |= (1 << 0))
#define MTC_SET_AES_READY(rp)                  ((rp)->MTC_CTRL2 |= (1 << 1))
#define MTC_SET_MULTI2_READY(rp)               ((rp)->MTC_CTRL2 |= (1 << 2))
#define MTC_SET_SMS4_READY(rp)                 ((rp)->MTC_CTRL2 |= (1 << 3))

#define MTC_SET_CW_ENABLE(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 0))
#define MTC_SET_ONE_ROUND(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 1))
#define MTC_SET_TWO_ROUND(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 2))
#define MTC_SET_THREE_ROUND(rp)                ((rp)->MTC_CW_SELECT |= (1 << 3))
#define MTC_SET_FOUR_ROUND(rp)                 ((rp)->MTC_CW_SELECT |= (1 << 12))
#define MTC_SET_FIVE_ROUND(rp)                 ((rp)->MTC_CW_SELECT |= (1 << 13))

#define MTC_SET_ONE_ROUND_READY(rp)            ((rp)->MTC_CA_MODE |= (1 << 3))
#define MTC_SET_TWO_ROUND_READY(rp)            ((rp)->MTC_CA_MODE |= (1 << 2) | (1 << 3))
#define MTC_SET_THREE_ROUND_READY(rp)          ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3))
#define MTC_SET_FOUR_ROUND_READY(rp)           ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6))
#define MTC_SET_FIVE_ROUND_READY(rp)           ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7))

#define MTC_CLR_CTRL1(rp)                      ((rp)->MTC_CTRL1 = 0)
#define MTC_CLR_CTRL2(rp)                      ((rp)->MTC_CTRL2 = 0)
#define MTC_ALL_INT_DISABLE(rp)                ((rp)->MTC_INT_EN = 0)
#define MTC_CLR_ALL_INT_STATUS(rp)             ((rp)->MTC_INT = 0xffffffff)
#define MTC_CLR_CA_MODE_REG(rp)                ((rp)->MTC_CA_MODE = 0)
#define MTC_CLR_CW_SELECT_REG(rp)              ((rp)->MTC_CW_SELECT = 0)

#define MTC_SET_CA_DATA_0(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x00) = value)
#define MTC_SET_CA_DATA_1(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x04) = value)
#define MTC_SET_CA_DATA_2(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x08) = value)
#define MTC_SET_CA_DATA_3(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x0c) = value)
#define MTC_SET_CA_DATA_4(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x10) = value)
#define MTC_SET_CA_DATA_5(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x14) = value)

#define MTC_SET_CA_DATA_DCW_0(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x00) = value)
#define MTC_SET_CA_DATA_DCW_1(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x04) = value)
#define MTC_SET_CA_DATA_DCW_2(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x08) = value)
#define MTC_SET_CA_DATA_DCW_3(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x0c) = value)

#ifdef __cplusplus
}
#endif

#endif

