#ifndef _GX_MTC_CORE_
#define _GX_MTC_CORE_

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MTC_Regs_s
{
	unsigned int     MTC_CTRL1;                   // 0x00
	unsigned int     MTC_KEY1_L;                  // 0x04
	unsigned int     MTC_KEY1_H;                  // 0x08
	unsigned int     MTC_KEY2_L;                  // 0x0c
	unsigned int     MTC_KEY2_H;                  // 0x10
	unsigned int     MTC_KEY3_L;                  // 0x14
	unsigned int     MTC_KEY3_H;                  // 0x18
	unsigned int     MTCR_SDR_START_ADDR;         // 0x1c
	unsigned int     MTCW_SDR_START_ADDR;         // 0x20
	unsigned int     MTCR_SDR_DATA_LEN;           // 0x24
	unsigned int     MTC_CTRL2;                   // 0x28
	unsigned int     MTC_KEY4_L;                  // 0x2c
	unsigned int     MTC_KEY4_H;                  // 0x30
	unsigned int     MTC_COUNTER3;                // 0x34
	unsigned int     MTC_COUNTER2;                // 0x38
	unsigned int     MTC_COUNTER1;                // 0x3c
	unsigned int     MTC_COUNTER0;                // 0x40
	unsigned int     MTC_IV3;                     // 0x44
	unsigned int     MTC_IV2;                     // 0x48
	unsigned int     MTC_IV1;                     // 0x4c
	unsigned int     MTC_IV0;                     // 0x50
	unsigned int     MULTI_DATA_KEY_H;            // 0x54
	unsigned int     MULTI_DATA_KEY_L;            // 0x58
	unsigned int     MTC_CA_MODE;                 // 0x5c
	unsigned int     MTC_RST_CTONRL;              // 0x60
	unsigned int     MTC_reveser1[11];            // 0x64~0x8c
	unsigned int     MTC_CA_ADDR;                 // 0x90
	unsigned int     MTC_reveser2[2];             // 0x94~0x98
	unsigned int     MTC_NDS_CONTROL_SET;         // 0x9c
	unsigned int     MTC_NDS_KEY_GEN;             // 0xa0
	unsigned int     MTC_NDS_KEY_WRITE_ADDR;      // 0xa4
	unsigned int     MTC_NDS_KEY_FULL;            // 0xa8
	unsigned int     MTC_MEMORY_KEY_WRITE_ADDR;   // 0xac
	unsigned int     MTC_reverser3[28];           // 0xb0~0x11c
	unsigned int     MTC_CW_SELECT;               // 0x120
	unsigned int     MTC_CA_NONCE_3;              // 0x124
	unsigned int     MTC_CA_NONCE_2;              // 0x128
	unsigned int     MTC_CA_NONCE_1;              // 0x12c
	unsigned int     MTC_CA_NONCE_0;              // 0x130
	unsigned int     MTC_CA_DA_3;                 // 0x134
	unsigned int     MTC_CA_DA_2;                 // 0x138
	unsigned int     MTC_CA_DA_1;                 // 0x13c
	unsigned int     MTC_CA_DA_0;                 // 0x140
	unsigned int     reverser4[3];                // 0x144~0x14c
	unsigned int     MTC_CA_K3_GEN;               // 0x150
	unsigned int     reverser5[3];                // 0x154~0x15c
	unsigned int     MTC_INT_EN;                  // 0x160
	unsigned int     MTC_INT;                     // 0x164
	unsigned int     reverser6[6];                // 0x168~0x17c
	unsigned int     MTC_IV2_3;                   // 0x180
	unsigned int     MTC_IV2_2;                   // 0x184
	unsigned int     MTC_IV2_1;                   // 0x188
	unsigned int     MTC_IV2_0;                   // 0x18c
	unsigned int     reverser7[28];               // 0x190~0x1fc
	unsigned int     MTC_CA_DSK_5;                // 0x200
	unsigned int     MTC_CA_DSK_4;                // 0x204
	unsigned int     MTC_CA_DSK_3;                // 0x208
	unsigned int     MTC_CA_DSK_2;                // 0x20c
	unsigned int     MTC_CA_DSK_1;                // 0x210
	unsigned int     MTC_CA_DSK_0;                // 0x214
	unsigned int     reverser8[2];                // 0x218~0x21c
	unsigned int     MTC_CA_DCK_5;                // 0x220
	unsigned int     MTC_CA_DCK_4;                // 0x224
	unsigned int     MTC_CA_DCK_3;                // 0x228
	unsigned int     MTC_CA_DCK_2;                // 0x22c
	unsigned int     MTC_CA_DCK_1;                // 0x230
	unsigned int     MTC_CA_DCK_0;                // 0x234
	unsigned int     reverser9[2];                // 0x238~0x23c
	unsigned int     MTC_CA_DCW_5;                // 0x240
	unsigned int     MTC_CA_DCW_4;                // 0x244
	unsigned int     MTC_CA_DCW_3;                // 0x248
	unsigned int     MTC_CA_DCW_2;                // 0x24c
	unsigned int     MTC_CA_DCW_1;                // 0x250
	unsigned int     MTC_CA_DCW_0;                // 0x254
	unsigned int     reverserA[2];                // 0x258~0x25c
	unsigned int     MTC_CA_VCW_5;                // 0x260
	unsigned int     MTC_CA_VCW_4;                // 0x264
	unsigned int     MTC_CA_VCW_3;                // 0x268
	unsigned int     MTC_CA_VCW_2;                // 0x26c
	unsigned int     MTC_CA_VCW_1;                // 0x270
	unsigned int     MTC_CA_VCW_0;                // 0x274
	unsigned int     reverserB[2];                // 0x278~0x27c
	unsigned int     MTC_CA_MCW_5;                // 0x280
	unsigned int     MTC_CA_MCW_4;                // 0x284
	unsigned int     MTC_CA_MCW_3;                // 0x288
	unsigned int     MTC_CA_MCW_2;                // 0x28c
	unsigned int     MTC_CA_MCW_1;                // 0x290
	unsigned int     MTC_CA_MCW_0;                // 0x294
}MTC_Regs_t;

#define KEY_SELECT_KEY_IPBUS_SET                    0
#define KEY_SELECT_FROM_OTP                         1
#define KEY_SELECT_NDS_KEY_MODULE                   2
#define KEY_SELECT_CDCAS_KEY_AES_CP                 3
#define KEY_SELECT_CDCAS_KEY_TDES_CP                4
#define KEY_SELECT_MULTI_LAYER_KEY                  5

#define MTC_SET_ENCRYPT(rp)                    ((rp)->MTC_CTRL1 |= (1 << 1))
#define MTC_SET_DECRYPT(rp)                    ((rp)->MTC_CTRL1 &= ~(1 << 1))

#define MTC_SET_ALGO_DES_3DES(rp)              ((rp)->MTC_CTRL1 |= (1 << 0))
#define MTC_SET_ALGO_AES128(rp)                ((rp)->MTC_CTRL1 |= ((0 << 3) | (1 << 5)))
#define MTC_SET_ALGO_AES192(rp)                ((rp)->MTC_CTRL1 |= ((1 << 3) | (1 << 5)))
#define MTC_SET_ALGO_AES256(rp)                ((rp)->MTC_CTRL1 |= ((2 << 3) | (1 << 5)))
#define MTC_SET_ALGO_MULTI2(rp)                ((rp)->MTC_CTRL1 |= (2 << 5))
#define MTC_SET_ALGO_SMS4(rp)                  ((rp)->MTC_CTRL1 |= (3 << 5))
#define MTC_SET_SUB_MODE(rp, value)            {(rp)->MTC_CTRL1 &= ~(7 << 7); (rp)->MTC_CTRL1 |= (value << 7);}
#define MTC_SET_KEY_SELECT(rp, value)          {(rp)->MTC_CTRL1 &= ~(7 << 17); (rp)->MTC_CTRL1 |= (value << 17);}

#define MTC_SET_KEY_31_0(rp, value)            ((rp)->MTC_KEY1_L = value)
#define MTC_SET_KEY_63_32(rp, value)           ((rp)->MTC_KEY1_H = value)
#define MTC_SET_KEY_95_64(rp, value)           ((rp)->MTC_KEY2_L = value)
#define MTC_SET_KEY_127_96(rp, value)          ((rp)->MTC_KEY2_H = value)
#define MTC_SET_KEY_159_128(rp, value)         ((rp)->MTC_KEY3_L = value)
#define MTC_SET_KEY_191_160(rp, value)         ((rp)->MTC_KEY3_H = value)
#define MTC_SET_KEY_223_192(rp, value)         ((rp)->MTC_KEY4_L = value)
#define MTC_SET_KEY_255_224(rp, value)         ((rp)->MTC_KEY4_H = value)

#define MTC_SET_COUNTER_31_0(rp, value)        ((rp)->MTC_COUNTER0 = value)
#define MTC_SET_COUNTER_63_32(rp, value)       ((rp)->MTC_COUNTER1 = value)
#define MTC_SET_COUNTER_95_64(rp, value)       ((rp)->MTC_COUNTER2 = value)
#define MTC_SET_COUNTER_127_96(rp, value)      ((rp)->MTC_COUNTER3 = value)

#define MTC_CLR_CTRL1(rp)                      ((rp)->MTC_CTRL1 = 0)
#define MTC_SET_KEY_READY(rp)                  ((rp)->MTC_CTRL1 |= (1 << 2))
#define MTC_CLR_KEY_READY(rp)                  ((rp)->MTC_CTRL1 &= ~(1 << 2))

#define MTC_SET_DES_KEY_HIGH_BIT(rp)           ((rp)->MTC_CTRL1 |= (1 << 20))
#define MTC_SET_DES_KEY_LOW_BIT(rp)            ((rp)->MTC_CTRL1 &= ~(1 << 20))

#define MTC_SET_READ_BUF_ADDR(rp, value)       ((rp)->MTCR_SDR_START_ADDR = value & 0x7fffffff)
#define MTC_SET_WRITE_BUF_ADDR(rp, value)      ((rp)->MTCW_SDR_START_ADDR = value & 0x7fffffff)
#define MTC_SET_READ_BUF_LEN(rp, value)        ((rp)->MTCR_SDR_DATA_LEN = value & 0x1fffff)
#define MTC_MAX_BUF_LEN                        ((unsigned int)0x1fffff)

#define MTC_SET_DES_READY(rp)                  ((rp)->MTC_CTRL2 |= (1 << 0))
#define MTC_SET_AES_READY(rp)                  ((rp)->MTC_CTRL2 |= (1 << 1))
#define MTC_SET_MULTI2_READY(rp)               ((rp)->MTC_CTRL2 |= (1 << 2))
#define MTC_SET_SMS4_READY(rp)                 ((rp)->MTC_CTRL2 |= (1 << 3))
#define MTC_SET_NDS_READY(rp)                  ((rp)->MTC_CTRL2 |= (1 << 4))
#define MTC_CLR_CTRL2(rp)                      ((rp)->MTC_CTRL2 = 0)

#define MTC_ALL_INT_ENABLE(rp)                 ((rp)->MTC_INT_EN = 0xff)
#define MTC_ALL_INT_DISABLE(rp)                ((rp)->MTC_INT_EN = 0)
#define MTC_CLR_ALL_INT_STATUS(rp)             ((rp)->MTC_INT = 0xffffffff)

#define MTC_DATA_INT_ENABLE(rp)                ((rp)->MTC_INT_EN = (1 << 0))
#define MTC_DATA_INT_DISABLE(rp)               ((rp)->MTC_INT_EN &= ~(1 << 0))

#define MTC_ALL_INT_STATUS_GET(rp)             ((rp)->MTC_INT)
#define MTC_GET_DATA_INT_STATUS(rp)            (((rp)->MTC_INT >> 0) & 0x1)
#define MTC_GET_TDES_INT_STATUS(rp)            (((rp)->MTC_INT >> 2) & 0x1)
#define MTC_GET_AES_INT_STATUS(rp)             (((rp)->MTC_INT >> 3) & 0x1)
#define MTC_CLR_DATA_INT_STATUS(rp)            ((rp)->MTC_INT |= (1 << 0))

#define MTC_SET_CA_MODE(rp)                    ((rp)->MTC_CA_MODE |= (1 << 0))
#define MTC_CLR_CA_MODE_REG(rp)                ((rp)->MTC_CA_MODE = 0)
#define MTC_SET_DSK_READY(rp)                  ((rp)->MTC_CA_MODE |= (1 << 1))
#define MTC_SET_DCK_READY(rp)                  ((rp)->MTC_CA_MODE |= (1 << 2))
#define MTC_SET_DCW_READY(rp)                  ((rp)->MTC_CA_MODE |= (1 << 3))
#define MTC_SET_DVCW_READY(rp)                 ((rp)->MTC_CA_MODE |= (1 << 6))
#define MTC_SET_DMVW_READY(rp)                 ((rp)->MTC_CA_MODE |= (1 << 7))

#define MTC_SET_CW_ENABLE(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 0))
#define MTC_SET_CALC_KEY_MODE(rp)              ((rp)->MTC_CW_SELECT |= (1 << 9))
#define MTC_SET_ONE_ROUND(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 1))
#define MTC_SET_TWO_ROUND(rp)                  ((rp)->MTC_CW_SELECT |= (1 << 2))
#define MTC_SET_THREE_ROUND(rp)                ((rp)->MTC_CW_SELECT |= (1 << 3))
#define MTC_SET_FOUR_ROUND(rp)                 ((rp)->MTC_CW_SELECT |= (1 << 12))
#define MTC_SET_FIVE_ROUND(rp)                 ((rp)->MTC_CW_SELECT |= (1 << 13))
#define MTC_CLR_CW_SELECT_REG(rp)              ((rp)->MTC_CW_SELECT = 0)

#define MTC_READ_NDS_KEY_FULL(rp)              ((rp)->MTC_NDS_KEY_FULL & 0x1)
#define MTC_CLR_NDS_KEY_FULL(rp)               ((rp)->MTC_NDS_KEY_FULL &= ~(1 << 0))

#define MTC_NDS_KEY_WRITE_DONE_STATUS(rp)              (((rp)->MTC_NDS_KEY_GEN >> 7) & 0x1)
#define MTC_NDS_KEY_GEN_CLR(rp)                        ((rp)->MTC_NDS_KEY_GEN = 0)
#define MTC_NDS_UPDATE_KEY_CLR(rp)                     ((rp)->MTC_NDS_KEY_GEN &= ~(1 << 2))
#define MTC_NDS_UPDATE_KEY_SET(rp)                     ((rp)->MTC_NDS_KEY_GEN |=  (1 << 2))
#define MTC_NDS_KEY_ADDR_CLR(rp)                       ((rp)->MTC_NDS_KEY_WRITE_ADDR &= ~(7 << 0))
#define MTC_NDS_KEY_ADDR_WRITE(rp, value)              ((rp)->MTC_NDS_KEY_WRITE_ADDR |= ((value & 0x7) << 0))
#define MTC_NDS_CONTROL_UNITWRITE_SET(rp)              ((rp)->MTC_NDS_KEY_GEN |=  (1 << 8))
#define MTC_NDS_CONTROL_UNITWRITE_CLR(rp)              ((rp)->MTC_NDS_KEY_GEN &= ~(1 << 8))
#define MTC_NDS_CONTROL_UNITWRITE_ADDR_CLR(rp)         ((rp)->MTC_NDS_KEY_GEN &= ~(7 << 9))
#define MTC_NDS_CONTROL_UNITWRITE_ADDR_SET(rp, value)  ((rp)->MTC_NDS_KEY_GEN |=  ((value & 0x7) << 9))
#define MTC_NDS_KEY_TABLE_SELECT_SET(rp)               ((rp)->MTC_NDS_KEY_GEN |=  (1 << 3))
#define MTC_NDS_KEY_TABLE_SELECT_CLR(rp)               ((rp)->MTC_NDS_KEY_GEN &= ~(1 << 3))
#define MTC_NDS_KEY_TABLE_SELECT(rp, value)            ((rp)->MTC_NDS_KEY_GEN |=  ((value & 0x7) << 4))

#define MTC_NDS_CONTROL_SET_CLR(rp)                    ((rp)->MTC_NDS_CONTROL_SET = 0)
#define MTC_NDS_CONTROL_CONFIG_ALGO(rp, value)         ((rp)->MTC_NDS_CONTROL_SET |= (value << 8))
#define MTC_NDS_CONTROL_CONFIG_SUBMODE(rp, value)      ((rp)->MTC_NDS_CONTROL_SET |= (value << 12))
#define MTC_NDS_CONTROL_CONFIG_RESIDE(rp, value)       ((rp)->MTC_NDS_CONTROL_SET |= (value << 16))
#define MTC_NDS_CONTROL_CONFIG_SHORT(rp, value)        ((rp)->MTC_NDS_CONTROL_SET |= (value << 20))
#define MTC_NDS_CONTROL_SET_ENCRYPT(rp)                ((rp)->MTC_NDS_CONTROL_SET |= (1 << 0))
#define MTC_NDS_CONTROL_SET_DECRYPT(rp)                ((rp)->MTC_NDS_CONTROL_SET |= (1 << 1))

#define MTC_SET_CA_ADDR_3211_ODD_FIRST(rp, id)        ((rp)->MTC_CA_ADDR = ((0xa0 + id * 2) << 16) | (0xa0 + id * 2 + 1))
#define MTC_SET_CA_ADDR_3211_EVEN_FIRST(rp, id)         ((rp)->MTC_CA_ADDR = ((0xa0 + id * 2 + 1) << 16) | (0xa0 + id * 2))
#define MTC_SET_CA_ADDR_6605S_CAS2_ODD_FIRST(rp, id)  ((rp)->MTC_CA_ADDR = ((0x29 + id * 4) << 16) | (0x29 + id * 4 + 2))
#define MTC_SET_CA_ADDR_6605S_CAS2_EVEN_FIRST(rp, id)   ((rp)->MTC_CA_ADDR = ((0x29 + id * 4 + 2) << 16) | (0x29 + id * 4))
#define MTC_SET_CA_ADDR_6605S_DES_EVEN_FIRST(rp, id)   ((rp)->MTC_CA_ADDR = ((0x28 + id * 4) << 16) | (0x28 + id * 4 + 2))
#define MTC_SET_CA_ADDR_6605S_DES_ODD_FIRST(rp, id)    ((rp)->MTC_CA_ADDR = ((0x28 + id * 4 + 2) << 16) | (0x28 + id * 4))

#define MTC_RST_CTL_5_SET(rp)                    ((rp)->MTC_RST_CTONRL |=  (1 << 5))
#define MTC_RST_CTL_5_CLR(rp)                    ((rp)->MTC_RST_CTONRL &= ~(1 << 5))

#define MTC_SET_CA_DATA_0(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x00) = value)
#define MTC_SET_CA_DATA_1(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x04) = value)
#define MTC_SET_CA_DATA_2(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x08) = value)
#define MTC_SET_CA_DATA_3(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x0c) = value)
#define MTC_SET_CA_DATA_4(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x10) = value)
#define MTC_SET_CA_DATA_5(base_addr, value)    (*(volatile unsigned int *)(base_addr + 0x14) = value)

#define MTC_SET_ONE_ROUND_READY(rp)            ((rp)->MTC_CA_MODE |= (1 << 3))
#define MTC_SET_TWO_ROUND_READY(rp)            ((rp)->MTC_CA_MODE |= (1 << 2) | (1 << 3))
#define MTC_SET_THREE_ROUND_READY(rp)          ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3))
#define MTC_SET_FOUR_ROUND_READY(rp)           ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6))
#define MTC_SET_FIVE_ROUND_READY(rp)           ((rp)->MTC_CA_MODE |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7))


#ifdef __cplusplus
}
#endif

#endif
