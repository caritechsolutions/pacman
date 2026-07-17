#include "board-common.h"
#include "cpu_config.h"
#include "interrupt.h"
#include DDR_PAR_INCLUDE

/* board extend func/data */
struct board_extend_s g_board_extend = {
	.p_ex = NULL,
	.func_ex = NULL,
	.func_usb_update = NULL,
};

/* multipin & gpio config init */
#define ARRAY_MAX_COUNT	1000

extern struct mulpin_config_s mulpin_table[];
extern struct gpio_entry_bootloader g_gpio_table[];
static int array_count = 0;
struct mulpin_module_verify_s{
	unsigned short num;
	unsigned char fun;
};

#define MD_VERIFY_COUNT_X 3
#define MD_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s md[MD_VERIFY_COUNT_X][MD_VERIFY_COUNT_Y] = {
	{
		{0,   1},//MD
		{1,   1},//MDC
	},
	{
		{7,   5},//MD
		{8,   5},//MDC
	},
	{
		{87,  1},//MD
		{90,  1},//MDC
	}
};

#define RM_VERIFY_COUNT_X 2
#define RM_VERIFY_COUNT_Y 7
static struct mulpin_module_verify_s rm[RM_VERIFY_COUNT_X][RM_VERIFY_COUNT_Y] = {
	{
		{2,   1},//RMCRSDV
		{3,   1},//RMTXEN
		{4,   1},//RMTXD1
		{5,   1},//RMTXD0
		{6,   1},//RMCLK
		{7,   1},//RMRXD1
		{8,   1},//RMRXD0
	},
	{
		{78,  2},//RMCRSDV
		{81,  2},//RMTXEN
		{82,  2},//RMTXD1
		{83,  2},//RMTXD0
		{84,  2},//RMCLK
		{85,  2},//RMRXD1
		{86,  2},//RMRXD0
	}
};

#define SDIO_VERIFY_COUNT_X 2
#define SDIO_VERIFY_COUNT_Y 9
static struct mulpin_module_verify_s sdio[SDIO_VERIFY_COUNT_X][SDIO_VERIFY_COUNT_Y] = {
	{
		{0,   2},//SDIO_CLK
		{1,   2},//SDIO_CMD
		{2,   2},//SDIO_DATA[0]
		{3,   2},//SDIO_DATA[1]
		{4,   2},//SDIO_DATA[2]
		{5,   2},//SDIO_DATA[3]
		{6,   2},//SDIO_PWR
		{7,   2},//SDIO_CD
		{8,   2},//SDIO_WP
	},
	{
		{64,  1},//SDIO_CLK
		{65,  1},//SDIO_CMD
		{66,  1},//SDIO_DATA[0]
		{67,  1},//SDIO_DATA[1]
		{68,  1},//SDIO_DATA[2]
		{69,  1},//SDIO_DATA[3]
		{63,  1},//SDIO_PWR
		{70,  1},//SDIO_CD
		{71,  1},//SDIO_WP
	}
};

#define DBG_VERIFY_COUNT_X 2
#define DBG_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s dbg[DBG_VERIFY_COUNT_X][DBG_VERIFY_COUNT_Y] = {
	{
		{12,  1},//DBGTDI
		{13,  1},//DBGTDO
		{14,  1},//DBGTMS
		{15,  1},//DBGTCK
		{16,  1},//DBGTRST
	},
	{
		{22,  6},//DBGTDI
		{23,  4},//DBGTDO
		{24,  4},//DBGTMS
		{25,  4},//DBGTCK
		{26,  4},//DBGTRST
	}
};

#define NANDFLASH_VERIFY_COUNT_X 2
#define NANDFLASH_VERIFY_COUNT_Y 13
static struct mulpin_module_verify_s nandflash[NANDFLASH_VERIFY_COUNT_X][NANDFLASH_VERIFY_COUNT_Y] = {
	{
		{17,  3},//NFRDY0
		{18,  3},//NFOE
		{19,  3},//NFCLE
		{20,  3},//NFALE
		{21,  3},//NFWE
		{22,  3},//NFDAT7
		{23,  3},//NFDAT6
		{24,  3},//NFDAT5
		{25,  3},//NFDAT4
		{26,  3},//NFDAT3
		{27,  3},//NFDAT2
		{28,  3},//NFDAT1
		{29,  3},//NFDAT0
	},
	{
		{91,  1},//NFRDY0
		{92,  1},//NFOE
		{93,  1},//NFCLE
		{94,  1},//NFALE
		{95,  1},//NFWE
		{96,  1},//NFDAT7
		{97,  1},//NFDAT6
		{98,  1},//NFDAT5
		{99,  1},//NFDAT4
		{100, 1},//NFDAT3
		{101, 1},//NFDAT2
		{102, 1},//NFDAT1
		{103, 1},//NFDAT0
	}
};

#define SPI_VERIFY_COUNT_X 2
#define SPI_VERIFY_COUNT_Y 6
static struct mulpin_module_verify_s spi[SPI_VERIFY_COUNT_X][SPI_VERIFY_COUNT_Y] = {
	{
		{17,  4},//SPIHOLD
		{18,  4},//SPICLK
		{19,  4},//SPIMOSI
		{20,  4},//SPICSN
		{21,  4},//SPIMISO
		{22,  4},//SPIWP
	},
	{
		{91,  2},//SPIHOLD
		{92,  2},//SPICLK
		{93,  2},//SPIMOSI
		{94,  2},//SPICSN
		{95,  2},//SPIMISO
		{96,  2},//SPIWP
	}
};

#define NF_SPI_VERIFY_COUNT_X 2
#define NF_SPI_VERIFY_COUNT_Y 6
static struct mulpin_module_verify_s nf_spi[NF_SPI_VERIFY_COUNT_X][NF_SPI_VERIFY_COUNT_Y] = {
	{
		{17,  5},//NF_SPIHOLD
		{18,  5},//NF_SPICLK
		{19,  5},//NF_SPIMOSI
		{20,  5},//NF_SPICSN
		{21,  5},//NF_SPIMISO
		{22,  5},//NF_SPIWP
	},
	{
		{91,  3},//NF_SPIHOLD
		{92,  3},//NF_SPICLK
		{93,  3},//NF_SPIMOSI
		{94,  3},//NF_SPICSN
		{95,  3},//NF_SPIMISO
		{96,  3},//NF_SPIWP
	}
};

#define UART2_VERIFY_COUNT_X 2
#define UART2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s uart2[UART2_VERIFY_COUNT_X][UART2_VERIFY_COUNT_Y] = {
	{
		{18,  6},//UART2TX
		{19,  6},//UART2RX
	},
	{
		{65,  2},//UART2TX
		{66,  2},//UART2RX
	}
};

#define I2C2_VERIFY_COUNT_X 2
#define I2C2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2c2[I2C2_VERIFY_COUNT_X][I2C2_VERIFY_COUNT_Y] = {
	{
		{20,  6},//SDA2
		{21,  6},//SCL2
	},
	{
		{55,  1},//SDA2
		{56,  1},//SCL2
	}
};


#define SMARTCARD_VERIFY_COUNT_X 3
#define SMARTCARD_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s smartcard[SMARTCARD_VERIFY_COUNT_X][SMARTCARD_VERIFY_COUNT_Y] = {
	{
		{22,  2},//SC1CLK
		{23,  2},//SC1RST
		{24,  2},//SC1PWD
		{25,  2},//SC1CD
		{26,  2},//SC1DATA
	},
	{
		{36,  2},//SC1CLK
		{37,  2},//SC1RST
		{38,  2},//SC1PWD
		{39,  2},//SC1CD
		{40,  2},//SC1DATA
	},
	{
		{41,  5},//SC1CLK
		{42,  5},//SC1RST
		{43,  5},//SC1PWD
		{44,  5},//SC1CD
		{45,  5},//SC1DATA
	}
};

#define TSO_VERIFY_COUNT_X 2
#define TSO_VERIFY_COUNT_Y 10
static struct mulpin_module_verify_s tso[TSO_VERIFY_COUNT_X][TSO_VERIFY_COUNT_Y] = {
	{
		{31,  1},//TSODATA7
		{32,  1},//TSODATA6
		{33,  1},//TSODATA5
		{34,  1},//TSODATA4
		{35,  1},//TSODATA3
		{36,  1},//TSODATA2
		{37,  1},//TSODATA1
		{38,  1},//TSODATA0
		{39,  1},//TSOCLK
		{40,  1},//TSOSYNC
	},
	{
		{41,  1},//TSODATA7
		{42,  1},//TSODATA6
		{43,  1},//TSODATA5
		{44,  1},//TSODATA4
		{45,  1},//TSODATA3
		{46,  1},//TSODATA2
		{47,  1},//TSODATA1
		{48,  1},//TSODATA0
		{49,  1},//TSOCLK
		{50,  1},//TSOSYNC
	}
};

#define I2S_VERIFY_COUNT_X 2
#define I2S_VERIFY_COUNT_Y 4
static struct mulpin_module_verify_s i2s[I2S_VERIFY_COUNT_X][I2S_VERIFY_COUNT_Y] = {
	{
		{13,  1},//I2SDATA
		{14,  1},//I2SBCK
		{15,  1},//I2SLRCK
		{16,  1},//I2SMCK
	},
	{
		{56,  1},//I2SDATA
		{57,  1},//I2SBCK
		{58,  1},//I2SLRCK
		{59,  1},//I2SMCK
	}
};

#define UART1_VERIFY_COUNT_X 2
#define UART1_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s uart1[UART1_VERIFY_COUNT_X][UART1_VERIFY_COUNT_Y] = {
	{
		{6,  2},//UART1TX
		{7,  2},//UART1RX
	},
	{
		{54,  1},//UART1TX
		{55,  1},//UART1RX
	}
};

#define SUART_VERIFY_COUNT_X 4
#define SUART_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s suart[SUART_VERIFY_COUNT_X][SUART_VERIFY_COUNT_Y] = {
	{
		{6,   3},//SUARTTX
		{7,   3},//SUARTRX
	},
	{
		{17,  4},//SUARTTX
		{18,  5},//SUARTRX
	},
	{
		{35,  5},//SUARTTX
		{36,  5},//SUARTRX
	},
	{
		{54,  2},//SUARTTX
		{55,  2},//SUARTRX
	}
};

#define AUART_VERIFY_COUNT_X 4
#define AUART_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s auart[AUART_VERIFY_COUNT_X][AUART_VERIFY_COUNT_Y] = {
	{
		{6,   4},//AUARTTX
		{7,   4},//AUARTRX
	},
	{
		{17,  5},//AUARTTX
		{18,  6},//AUARTRX
	},
	{
		{35,  6},//AUARTTX
		{36,  6},//AUARTRX
	},
	{
		{54,  3},//AUARTTX
		{55,  3},//AUARTRX
	}
};

#define I2C1_VERIFY_COUNT_X 2
#define I2C1_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2c1[I2C1_VERIFY_COUNT_X][I2C1_VERIFY_COUNT_Y] = {
	{
		{37,  1},//SDA1
		{38,  1},//SCL1
	},
	{
		{46,  1},//SDA1
		{47,  1},//SCL1
	}
};

#define I2CT_VERIFY_COUNT_X 2
#define I2CT_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2ct[I2CT_VERIFY_COUNT_X][I2CT_VERIFY_COUNT_Y] = {
	{
		{37,  2},//SDAT
		{38,  2},//SCLT
	},
	{
		{48,  2},//SDAT
		{49,  2},//SCLT
	}
};

#define AGC_VERIFY_COUNT_X 2
#define AGC_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s agc[AGC_VERIFY_COUNT_X][AGC_VERIFY_COUNT_Y] = {
	{
		{34,  1},//AGC
	},
	{
		{50,  1},//AGC
	}
};

#define FM_AGC_VERIFY_COUNT_X 2
#define FM_AGC_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s fm_agc[FM_AGC_VERIFY_COUNT_X][FM_AGC_VERIFY_COUNT_Y] = {
	{
		{34,  4},//FM_AGC
	},
	{
		{50,  2},//FM_AGC
	}
};

#define ADBG_VERIFY_COUNT_X 2
#define ADBG_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s adbg[ADBG_VERIFY_COUNT_X][ADBG_VERIFY_COUNT_Y] = {
	{
		{20,  3},//ADBGTDI
		{21,  3},//ADBGTDO
		{22,  3},//ADBGTMS
		{23,  3},//ADBGTCK
		{24,  3},//ADBGTRST
	},
	{
		{26,  7},//ADBGTDI
		{27,  7},//ADBGTDO
		{28,  7},//ADBGTMS
		{29,  7},//ADBGTCK
		{30,  7},//ADBGTRST
	}
};

#define CEC_VERIFY_COUNT_X 3
#define CEC_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s cec[CEC_VERIFY_COUNT_X][CEC_VERIFY_COUNT_Y] = {
	{
		{18,  3},//CEC
	},
	{
		{24,  6},//CEC
	},
	{
		{25,  1},//CEC
	}
};

static int module_funs_select_compare(struct mulpin_module_verify_s* p)
{
	int i = 0;
	for(i = 0; i < array_count; i++)
	{
		if(p->num == mulpin_table[i].pin_id)
		{
			if(p->fun == mulpin_table[i].fun)
				return 0;
			else
				return -1;
		}
	}
	return -1;
}

static int mulpin_verify(int x, int y, struct mulpin_module_verify_s *p)
{
	int i = 0;
	int j = 0;
	int num = 0;
	int ret = 0;

	for (j = 0; j < y; j++) {
		for (num = 0; num < x; num++) {
			for (i = num + 1; i < x; i++) {
				if((module_funs_select_compare(&p[num * y + j]) == 0) && (module_funs_select_compare(&p[i * y  + j]) == 0)) {
					printf("mulpin verify error:(num = %d) and (num = %d) is both enable.\n",\
							p[num * y + j].num, p[i * y + j].num);
					ret = -1;
				}
			}
		}
	}

	return ret;
}

static int mulpin_module_verify(void)
{
	int ret = 0;

#if 0
	//wp_hold verify
	if (mulpin_verify(WP_HOLD_VERIFY_COUNT_X, WP_HOLD_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)wp_hold)) {
		ret = -1;
		printf("WP HOLD verify end.\n");
	}

	//smartcard verify
	if (mulpin_verify(SMARTCARD_VERIFY_COUNT_X, SMARTCARD_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)smartcard)) {
		ret = -1;
		printf("SMARTCARD verify end.\n");
	}

	//i2s verify
	if (mulpin_verify(I2S_VERIFY_COUNT_X, I2S_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2s)) {
		ret = -1;
		printf("I2S verify end.\n");
	}

	//uart1 verify
	if (mulpin_verify(UART1_VERIFY_COUNT_X, UART1_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)uart1)) {
		ret = -1;
		printf("UART1 verify end.\n");
	}

	//uart2 verify
	if (mulpin_verify(UART2_VERIFY_COUNT_X, UART2_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)uart2)) {
		ret = -1;
		printf("UART2 verify end.\n");
	}

	//i2c1 verify
	if (mulpin_verify(I2C1_VERIFY_COUNT_X, I2C1_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2c1)) {
		ret = -1;
		printf("I2C1 verify end.\n");
	}

	//i2c2 verify
	if (mulpin_verify(I2C2_VERIFY_COUNT_X, I2C2_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2c2)) {
		ret = -1;
		printf("I2C2 verify end.\n");
	}

	//i2c_t verify
	if (mulpin_verify(I2CT_VERIFY_COUNT_X, I2CT_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2ct)) {
		ret = -1;
		printf("I2C_T verify end.\n");
	}

	//agc verify
	if (mulpin_verify(AGC_VERIFY_COUNT_X, AGC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)agc)) {
		ret = -1;
		printf("AGC verify end.\n");
	}

	//fm_agc verify
	if (mulpin_verify(FM_AGC_VERIFY_COUNT_X, FM_AGC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)fm_agc)) {
		ret = -1;
		printf("FM_AGC verify end.\n");
	}

	//suart verify
	if (mulpin_verify(SUART_VERIFY_COUNT_X, SUART_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)suart)) {
		ret = -1;
		printf("SUART verify end.\n");
	}

	//auart verify
	if (mulpin_verify(AUART_VERIFY_COUNT_X, AUART_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)auart)) {
		ret = -1;
		printf("AUART verify end.\n");
	}

	//sdbg verify
	if (mulpin_verify(SDBG_VERIFY_COUNT_X, SDBG_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)sdbg)) {
		ret = -1;
		printf("SDBG verify end.\n");
	}

	//adbg verify
	if (mulpin_verify(ADBG_VERIFY_COUNT_X, ADBG_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)adbg)) {
		ret = -1;
		printf("ADBG verify end.\n");
	}

	//cec verify
	if (mulpin_verify(CEC_VERIFY_COUNT_X, CEC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)cec)) {
		ret = -1;
		printf("CEC verify end.\n");
	}

	//rm verify
	if (mulpin_verify(RM_VERIFY_COUNT_X, RM_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)rm)) {
		ret = -1;
		printf("RM verify end.\n");
	}
#endif

	return ret;
}

void mulpin_init(void)
{
	int i = 0;
	unsigned char act_fun;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if(mulpin_table[i].pin_id == ARRAY_END_FLAG_U16) {
			array_count = i;
			break;
		}
	}

	for(i = 0; i < array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG)
			padmux_set(mulpin_table[i].pin_id, mulpin_table[i].fun);
	}

	/* cmp the config fun & actual fun */
	for(i = 0; i < array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){
			act_fun = padmux_get(mulpin_table[i].pin_id);
			if (act_fun != mulpin_table[i].fun) {
				printf("mulpin config error:(pin_id=%d)config fun=%d, actual fun=%d.\n", \
						mulpin_table[i].pin_id, mulpin_table[i].fun, act_fun);
			}
		}
	}
#ifdef CONFIG_MULPIN_VERIFY
	/*module mulpin conflict verify*/
	if(mulpin_module_verify() != 0)
	{
		printf("please resolve the mulpin_table conflict error.\n");
		while(1);
	}
#endif
}


unsigned int get_gpio_entry_num(void)
{
	int i;
	unsigned int entry_num = 0;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if((g_gpio_table[i].vir_gpio & g_gpio_table[i].phy_gpio) == ARRAY_END_FLAG){
			entry_num = i;
			break;
		}
	}

	return entry_num;
}

void gpio_init(void)
{
	unsigned int i				= 0;
	unsigned int offs			= 0;		/* 0~31 */
	unsigned int entry_num			= 0;
	unsigned int gx_gpio_register		= 0;
	struct gpio_table_header* sram_gpio_table_header = (struct gpio_table_header*)GPIO_TABLE_START_ADDR;
	struct gpio_entry_bootloader* sram_entry = (struct gpio_entry_bootloader*)(GPIO_TABLE_START_ADDR + sizeof(struct gpio_table_header));

	entry_num = get_gpio_entry_num();

	/* initialize the header for later use */
	sram_gpio_table_header->magic     = GPIO_MAGIC;
	sram_gpio_table_header->entry_num = entry_num;
	if(entry_num == 0)
		sram_gpio_table_header->valid = 0;
	else
		sram_gpio_table_header->valid = 1;

	for (i = 0; i < entry_num; i++) {
		/* copy info into sram */
		sram_entry[i] = g_gpio_table[i];

		/* initialize gpio */
		if (!g_gpio_table[i].config_valid)
			continue;

		if (g_gpio_table[i].phy_gpio < 32) {
			gx_gpio_register = (unsigned int)(REG_BASE_GPIO1);
			offs = g_gpio_table[i].phy_gpio;
		} else if(g_gpio_table[i].phy_gpio < 64) {
			gx_gpio_register = (unsigned int)(REG_BASE_GPIO2);
			offs = g_gpio_table[i].phy_gpio - 32;
		} else if(g_gpio_table[i].phy_gpio < 96) {
			gx_gpio_register = (unsigned int)(REG_BASE_GPIO3);
			offs = g_gpio_table[i].phy_gpio - 64;
		} else if(g_gpio_table[i].phy_gpio < 128) {
			gx_gpio_register = (unsigned int)(REG_BASE_GPIO4);
			offs = g_gpio_table[i].phy_gpio - 96;
		} else
			printf("file(%s) line(%d):gpio parameters error.\n", __FILE__, __LINE__);

		if (g_gpio_table[i].io_mode) {
			*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_OUT) = 1 << offs;
			if (g_gpio_table[i].output_value)
				*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_HI) = 1 << offs;
			else
				*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_LO) = 1 << offs;
		} else
			*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_IN) = 1 << offs;
	}
}

int ts_mux_config(int ts_mux_in, int ts_in, int edge_select)
{
	switch(ts_mux_in) {
	case TS_MUX_DVB_TS_OUT:
		if (ts_in != TS_IN_TS0_P) {
			printf("TS_MUX_DVB_TS_OUT only support TS_IN_TS0_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~0x1;
		break;
	case TS_MUX_TSI_OUT:
		if (ts_in != TS_IN_TS0_P) {
			printf("TS_MUX_TSI_OUT only support TS_IN_TS0_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) |= 0x1;
		break;
	case TS_MUX_TS_S_1:
		if (ts_in == TS_IN_TS0_S)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) &= ~(0x1<<27);
		else if (ts_in == TS_IN_TS1_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) &= ~(0x1<<24);
		else {
			printf("TS_MUX_TS_S_1 only support TS_IN_TS0_S and TS_IN_TS1_P\n");
			return -1;
		}
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<15);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<15);
		break;
	case TS_MUX_TS_S_2:
		if (ts_in == TS_IN_TS0_S)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) |= (0x1<<27);
		else if (ts_in == TS_IN_TS2_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) &= ~(0x1<<25);
		else {
			printf("TS_MUX_TS_S_2 only support TS_IN_TS0_S and TS_IN_TS2_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<1);
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<19);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<19);
		break;
	case TS_MUX_TS_S_3:
		if (ts_in == TS_IN_TS3_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) &= ~(0x1<<26);
		else {
			printf("TS_MUX_TS_S_3 only support TS_IN_TS3_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<2);
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<28);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<28);
		break;
	case TS_MUX_TS_P_1:
		if (ts_in == TS_IN_TS1_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) |= (0x1<<24);
		else {
			printf("TS_MUX_TS_S_1 only support TS_IN_TS1_P\n");
			return -1;
		}
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<16);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<16);
		break;
	case TS_MUX_TS_P_2:
		if (ts_in == TS_IN_TS3_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) |= (0x1<<26);
		else {
			printf("TS_MUX_TS_P_2 only support TS_IN_TS3_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<2);
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<17);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<17);
		break;
	case TS_MUX_TS_P_3:
		if (ts_in == TS_IN_TS2_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x680) |= (0x1<<25);
		else {
			printf("TS_MUX_TS_P_3 only support TS_IN_TS2_P\n");
			return -1;
		}
		*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<1);
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<18);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<18);
		break;
	case TS_MUX_DVB_TS_OUT1:
		if (ts_in == TS_IN_TS2_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) |= (0x1<<1);
		else if (ts_in == TS_IN_TS3_P)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) |= (0x1<<2);
		else {
			printf("TS_MUX_DVB_TS_OUT1 only support TS_IN_TS2_P and TS_IN_TS3_P\n");
			return -1;
		}
		break;
	default:
		printf("TS_MUX_IN error\n");
		return -1;
	}

	return 0;
}
/*
 * Detailed explanation see the board-common.h
 * */
int ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select)
{
#ifndef CONFIG_CLOCK_PIDFILTER_DISABLE
	if (port_mode)
		*(volatile unsigned int*)(REG_BASE_PIDFILTER + 0x10d0) = pin_config;
#endif
	if(ts_port == 0){
		if(port_mode){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4800) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4800) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<8);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x14800) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x4800);
			if (port_mode == 1)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) &= ~(0x3);
			else if (port_mode == 2)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= 0x1;
			else if (port_mode == 3)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= 0x2;

		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<8);
		}
		if(valid_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<0);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<0);
		}
		if(sync_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<12);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<12);
		}
		if(endian_select){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<4);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<4);
		}
	} else if(ts_port == 1){
		if(port_mode){
			/* serial ts select input(clk/sync/valid/dat) */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4804) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4804) |= pin_config;
			/* serial ts enable */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<9);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x14804) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x4804);
			if (port_mode == 1)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) &= ~(0x3<<2);
			else if (port_mode == 2)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x1<<2);
			else if (port_mode == 3)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x2<<2);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<9);
		}

		if(valid_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<1);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<1);
		}

		if(sync_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<13);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<13);
		}

		if(endian_select){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<5);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<5);
		}
	} else if(ts_port == 2){
		if(port_mode){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<10);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x14808) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808);
			if (port_mode == 1)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) &= ~(0x3<<4);
			else if (port_mode == 2)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x1<<4);
			else if (port_mode == 3)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x2<<4);
		} else {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<10);
		}
		if(valid_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<2);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<2);
		}
		if(sync_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<14);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<14);
		}
		if(endian_select){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<6);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<6);
		}
	}else if(ts_port == 3){
		if(port_mode){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x480C) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x480C) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<11);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x1480c) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x480c);
			if (port_mode == 1)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) &= ~(0x3<<6);
			else if (port_mode == 2)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x1<<6);
			else if (port_mode == 3)
				*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8) |= (0x2<<6);
		} else {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<11);
		}

		if(valid_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<3);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<3);
		}
		if(sync_enable){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<15);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<15);
		}
		if(endian_select){
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<7);
		}else{
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(1<<7);
		}
	}else{
		printf("ts_port=%d not support, please input 0/1/2/3.\n", ts_port);
		return -1;
	}
	*(volatile unsigned int*)(REG_BASE_DEMUX + 0x136d4) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4);
	*(volatile unsigned int*)(REG_BASE_DEMUX + 0x136e8) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x36e8);
	return 0;
}

void dac_gain_control(void)
{
#ifdef CONFIG_ENABLE_GX_OTP
	int i = 0;
	unsigned char otp_data[2] = {0x0};
	unsigned char default_data[2] = {0xDA, 0xCB};
	unsigned int otp_addr[2] = {0x109, 0x10a};

	for (i = 0; i < 2; i++) {
		gx_otp_read(otp_addr[i], 1, &otp_data[i]);
		if (otp_data[i] == 0) {
			otp_data[i] = default_data[i];
			printf("\nwarning : use default dac gain = 0x%x !\n", otp_data[i]);
		}
	}
	REG_SET_FIELD(VDAC_BASE_ADDR + (0xA1<<2), 0xFF, otp_data[0], 0);
	if(0 == (otp_data[1] >> 4))
		otp_data[1] = default_data[1];
	REG_SET_FIELD(VDAC_BASE_ADDR + (0xA0<<2), 0xF0, otp_data[1] >> 4, 4);
#else
	printf("\n\nerror : configure DAC must set ENABLE_GX_OTP = y !\n\n");
	while(1);
#endif
}

extern enum vout_dac_case default_dac_case_vpu;
extern enum vout_dac_case default_dac_case_svpu;
void vout_init(void)
{
	int default_dac_mode_svpu = 0;

#ifdef CONFIG_ARCH_ARM_CANOPUS
	// set vdac
	*(unsigned int volatile *)(VDAC_BASE_ADDR + (0x00<<2)) = 0;
	*(unsigned int volatile *)(VDAC_BASE_ADDR + (0x00<<2)) = 0xC0;
#endif /*#ifdef CONFIG_ARCH_ARM_CANOPUS*/
	/* set svpu dac case */
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) &= ~(0x1f<<25);
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) |=  (default_dac_case_svpu<<25);
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) &= ~(0x1f<<17);
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) |=  (default_dac_mode_svpu<<17);
	/* set vpu dac case */
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x00) &= ~(0x1f<<25);
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x00) |=  (default_dac_case_vpu<<25);

	*(volatile unsigned int *)(SVPU_VOUT_BASE_ADDR + 0x110) = 0x10000001;
	*(volatile unsigned int *)(VPU_VOUT_BASE_ADDR  + 0x1b0) = 0x00163863;
	*(volatile unsigned int *)(VPU_VOUT_BASE_ADDR  + 0x1b8) = 0x00163863;
#if defined(CHIP_BOARD_3215) || defined(CHIP_BOARD_6133S) || defined(CHIP_BOARD_6633H) || defined(CHIP_BOARD_3203H) || defined(CHIP_BOARD_6113S) || defined(CHIP_BOARD_6633) || defined(CHIP_BOARD_3203)
	*(volatile unsigned int *)(VPU_BASE_ADDR + 0x134) = 0x00000018; // open only on video dac for cvbs
#endif
	dac_gain_control(); // set video dac gain to 19
	REG_SET_BIT(VDAC_BASE_ADDR + (0xA0<<2), 0);
	REG_SET_BIT(VDAC_BASE_ADDR + (0xA0<<2), 1);
}

enum video_dac_mode dac_config_mode = DAC_NOT_CONFIG;
void enable_dac(enum video_dac_mode mode)
{
	switch (mode){
	case ENABLE_CVBS:
	case ENABLE_CVBS_AND_YPBPR:
		dac_config_mode = mode;
		break;
	default:
		printf("warning: %s mode = %d, is not valid.\n", __func__, mode);
		break;
	}
}

/*
 * Weak default function for board specific operation
 * Please implement your own board specific operation in file board_init.c to do this.
 */
void __attribute__((weak)) board_operation(void)
{
}

int system_config_init(void)
{
}

/* denali_config.c / h5tq2g63bfr_h9.c */
unsigned int DENALI_CONFIG_CTL[159] __attribute__((section(".sram"))) = {
	DENALI_CTL_00_DATA,
	DENALI_CTL_01_DATA,
	DENALI_CTL_02_DATA,
	DENALI_CTL_03_DATA,
	DENALI_CTL_04_DATA,
	DENALI_CTL_05_DATA,
	DENALI_CTL_06_DATA,
	DENALI_CTL_07_DATA,
	DENALI_CTL_08_DATA,
	DENALI_CTL_09_DATA,
	DENALI_CTL_10_DATA,
	DENALI_CTL_11_DATA,
	DENALI_CTL_12_DATA,
	DENALI_CTL_13_DATA,
	DENALI_CTL_14_DATA,
	DENALI_CTL_15_DATA,
	DENALI_CTL_16_DATA,
	DENALI_CTL_17_DATA,
	DENALI_CTL_18_DATA,
	DENALI_CTL_19_DATA,
	DENALI_CTL_20_DATA,
	DENALI_CTL_21_DATA,
	DENALI_CTL_22_DATA,
	DENALI_CTL_23_DATA,
	DENALI_CTL_24_DATA,
	DENALI_CTL_25_DATA,
	DENALI_CTL_26_DATA,
	DENALI_CTL_27_DATA,
	DENALI_CTL_28_DATA,
	DENALI_CTL_29_DATA,
	DENALI_CTL_30_DATA,
	DENALI_CTL_31_DATA,
	DENALI_CTL_32_DATA,
	DENALI_CTL_33_DATA,
	DENALI_CTL_34_DATA,
	DENALI_CTL_35_DATA,
	DENALI_CTL_36_DATA,
	DENALI_CTL_37_DATA,
	DENALI_CTL_38_DATA,
	DENALI_CTL_39_DATA,
	DENALI_CTL_40_DATA,
	DENALI_CTL_41_DATA,
	DENALI_CTL_42_DATA,
	DENALI_CTL_43_DATA,
	DENALI_CTL_44_DATA,
	DENALI_CTL_45_DATA,
	DENALI_CTL_46_DATA,
	DENALI_CTL_47_DATA,
	DENALI_CTL_48_DATA,
	DENALI_CTL_49_DATA,
	DENALI_CTL_50_DATA,
	DENALI_CTL_51_DATA,
	DENALI_CTL_52_DATA,
	DENALI_CTL_53_DATA,
	DENALI_CTL_54_DATA,
	DENALI_CTL_55_DATA,
	DENALI_CTL_56_DATA,
	DENALI_CTL_57_DATA,
	DENALI_CTL_58_DATA,
	DENALI_CTL_59_DATA,
	DENALI_CTL_60_DATA,
	DENALI_CTL_61_DATA,
	DENALI_CTL_62_DATA,
	DENALI_CTL_63_DATA,
	DENALI_CTL_64_DATA,
	DENALI_CTL_65_DATA,
	DENALI_CTL_66_DATA,
	DENALI_CTL_67_DATA,
	DENALI_CTL_68_DATA,
	DENALI_CTL_69_DATA,
	DENALI_CTL_70_DATA,
	DENALI_CTL_71_DATA,
	DENALI_CTL_72_DATA,
	DENALI_CTL_73_DATA,
	DENALI_CTL_74_DATA,
	DENALI_CTL_75_DATA,
	DENALI_CTL_76_DATA,
	DENALI_CTL_77_DATA,
	DENALI_CTL_78_DATA,
	DENALI_CTL_79_DATA,
	DENALI_CTL_80_DATA,
	DENALI_CTL_81_DATA,
	DENALI_CTL_82_DATA,
	DENALI_CTL_83_DATA,
	DENALI_CTL_84_DATA,
	DENALI_CTL_85_DATA,
	DENALI_CTL_86_DATA,
	DENALI_CTL_87_DATA,
	DENALI_CTL_88_DATA,
	DENALI_CTL_89_DATA,
	DENALI_CTL_90_DATA,
	DENALI_CTL_91_DATA,
	DENALI_CTL_92_DATA,
	DENALI_CTL_93_DATA,
	DENALI_CTL_94_DATA,
	DENALI_CTL_95_DATA,
	DENALI_CTL_96_DATA,
	DENALI_CTL_97_DATA,
	DENALI_CTL_98_DATA,
	DENALI_CTL_99_DATA,
	DENALI_CTL_100_DATA,
	DENALI_CTL_101_DATA,
	DENALI_CTL_102_DATA,
	DENALI_CTL_103_DATA,
	DENALI_CTL_104_DATA,
	DENALI_CTL_105_DATA,
	DENALI_CTL_106_DATA,
	DENALI_CTL_107_DATA,
	DENALI_CTL_108_DATA,
	DENALI_CTL_109_DATA,
	DENALI_CTL_110_DATA,
	DENALI_CTL_111_DATA,
	DENALI_CTL_112_DATA,
	DENALI_CTL_113_DATA,
	DENALI_CTL_114_DATA,
	DENALI_CTL_115_DATA,
	DENALI_CTL_116_DATA,
	DENALI_CTL_117_DATA,
	DENALI_CTL_118_DATA,
	DENALI_CTL_119_DATA,
	DENALI_CTL_120_DATA,
	DENALI_CTL_121_DATA,
	DENALI_CTL_122_DATA,
	DENALI_CTL_123_DATA,
	DENALI_CTL_124_DATA,
	DENALI_CTL_125_DATA,
	DENALI_CTL_126_DATA,
	DENALI_CTL_127_DATA,
	DENALI_CTL_128_DATA,
	DENALI_CTL_129_DATA,
	DENALI_CTL_130_DATA,
	DENALI_CTL_131_DATA,
	DENALI_CTL_132_DATA,
	DENALI_CTL_133_DATA,
	DENALI_CTL_134_DATA,
	DENALI_CTL_135_DATA,
	DENALI_CTL_136_DATA,
	DENALI_CTL_137_DATA,
	DENALI_CTL_138_DATA,
	DENALI_CTL_139_DATA,
	DENALI_CTL_140_DATA,
	DENALI_CTL_141_DATA,
	DENALI_CTL_142_DATA,
	DENALI_CTL_143_DATA,
	DENALI_CTL_144_DATA,
	DENALI_CTL_145_DATA,
	DENALI_CTL_146_DATA,
	DENALI_CTL_147_DATA,
	DENALI_CTL_148_DATA,
	DENALI_CTL_149_DATA,
	DENALI_CTL_150_DATA,
	DENALI_CTL_151_DATA,
	DENALI_CTL_152_DATA,
	DENALI_CTL_153_DATA,
	DENALI_CTL_154_DATA,
	DENALI_CTL_155_DATA,
	DENALI_CTL_156_DATA,
	DENALI_CTL_157_DATA,
	DENALI_CTL_158_DATA
};

unsigned int DENALI_CONFIG_PHY[31]  __attribute__((section(".sram"))) = {
	DENALI_PHY_00_DATA , // DEN_PHY_DQ_TIMING_REG_0      : RW : 0 : 32 :=0x26272627
	DENALI_PHY_01_DATA , // DEN_PHY_DQS_TIMING_REG_0     : RW : 0 : 32 :=0x263a263a
	DENALI_PHY_02_DATA , // DEN_PHY_GATE_LPBK_CTRL_REG_0 : RW : 0 : 32 :=0x00d90060
	DENALI_PHY_03_DATA , // PHY_DLL_MASTER_CTRL_REG_0    : RW : 0 : 32 :=0x000000ff
	DENALI_PHY_04_DATA , // PHY_DLL_SLAVE_CTRL_REG_0     : RW : 0 : 32 :=0x25282528
	DENALI_PHY_05_DATA , // DEN_PHY_OBS_REG_0_0          : RD : 0 : 32 :=0x00000000
	DENALI_PHY_06_DATA , // DEN_PHY_DQ_TIMING_REG_1      : RW : 0 : 32 :=0x26272627
	DENALI_PHY_07_DATA , // DEN_PHY_DQS_TIMING_REG_1     : RW : 0 : 32 :=0x263a263a
	DENALI_PHY_08_DATA , // DEN_PHY_GATE_LPBK_CTRL_REG_1 : RW : 0 : 32 :=0x00d90060
	DENALI_PHY_09_DATA , // PHY_DLL_MASTER_CTRL_REG_1    : RW : 0 : 32 :=0x000000ff
	DENALI_PHY_10_DATA , // PHY_DLL_SLAVE_CTRL_REG_1     : RW : 0 : 32 :=0x25282528
	DENALI_PHY_11_DATA , // DEN_PHY_OBS_REG_0_1          : RD : 0 : 32 :=0x00000000
	DENALI_PHY_12_DATA , // DEN_PHY_DQ_TIMING_REG_2      : RW : 0 : 32 :=0x26272627
	DENALI_PHY_13_DATA , // DEN_PHY_DQS_TIMING_REG_2     : RW : 0 : 32 :=0x263a263a
	DENALI_PHY_14_DATA , // DEN_PHY_GATE_LPBK_CTRL_REG_2 : RW : 0 : 32 :=0x00d90060
	DENALI_PHY_15_DATA , // PHY_DLL_MASTER_CTRL_REG_2    : RW : 0 : 32 :=0x000000ff
	DENALI_PHY_16_DATA , // PHY_DLL_SLAVE_CTRL_REG_2     : RW : 0 : 32 :=0x25282528
	DENALI_PHY_17_DATA , // DEN_PHY_OBS_REG_0_2          : RD : 0 : 32 :=0x00000000
	DENALI_PHY_18_DATA , // DEN_PHY_DQ_TIMING_REG_3      : RW : 0 : 32 :=0x26272627
	DENALI_PHY_19_DATA , // DEN_PHY_DQS_TIMING_REG_3     : RW : 0 : 32 :=0x263a263a
	DENALI_PHY_20_DATA , // DEN_PHY_GATE_LPBK_CTRL_REG_3 : RW : 0 : 32 :=0x00d90060
	DENALI_PHY_21_DATA , // PHY_DLL_MASTER_CTRL_REG_3    : RW : 0 : 32 :=0x000000ff
	DENALI_PHY_22_DATA , // PHY_DLL_SLAVE_CTRL_REG_3     : RW : 0 : 32 :=0x25282528
	DENALI_PHY_23_DATA , // DEN_PHY_OBS_REG_0_3          : RD : 0 : 32 :=0x00000000
	DENALI_PHY_24_DATA , // DEN_PHY_CTRL_REG             : RW : 0 : 32 :=0x00004004
	DENALI_PHY_25_DATA , // DEN_PHY_PAD_TSEL_REG         : RW : 0 : 32 :=0x00010101
	DENALI_PHY_26_DATA ,
	DENALI_PHY_27_DATA ,
	DENALI_PHY_28_DATA ,
	DENALI_PHY_29_DATA ,
	DENALI_PHY_30_DATA
};

