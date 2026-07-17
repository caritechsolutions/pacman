#include "board-common.h"
#include "cpu_config.h"
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
	unsigned char side;
	unsigned short num;
	unsigned char fun;
};

#if 0
#define SDBG_VERIFY_COUNT_X 2
#define SDBG_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s sdbg[SDBG_VERIFY_COUNT_X][SDBG_VERIFY_COUNT_Y] = {
	{
		{_BOTTOM_,  42,  2},//SDBGTDI
		{_BOTTOM_,  43,  2},//SDBGTDO
		{_BOTTOM_,  44,  2},//SDBGTMS
		{_BOTTOM_,  45,  2},//SDBGTCK
		{_BOTTOM_,  46,  2},//SDBGTRST
	},
	{
		{_BOTTOM_,  47,  5},//SDBGTDI
		{_BOTTOM_,  48,  5},//SDBGTDO
		{_BOTTOM_,  49,  5},//SDBGTMS
		{_BOTTOM_,  50,  5},//SDBGTCK
		{_RIGHT_,   1,   5},//SDBGTRST
	}
};
#endif

#define SMARTCARD_VERIFY_COUNT_X 2
#define SMARTCARD_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s smartcard[SMARTCARD_VERIFY_COUNT_X][SMARTCARD_VERIFY_COUNT_Y] = {
	{
		{_BOTTOM_,  47,  3},//SC1CLK
		{_BOTTOM_,  48,  3},//SC1RST
		{_BOTTOM_,  49,  3},//SC1PWD
		{_BOTTOM_,  50,  3},//SC1CD
		{_RIGHT_,   1,   3},//SC1DATA
	},
	{
		{_BOTTOM_,  26,  0},//SC1CLK
		{_BOTTOM_,  27,  0},//SC1RST
		{_BOTTOM_,  28,  0},//SC1PWD
		{_BOTTOM_,  29,  0},//SC1CD
		{_BOTTOM_,  30,  0},//SC1DATA
	}
};

#define I2S_VERIFY_COUNT_X 3
#define I2S_VERIFY_COUNT_Y 4
static struct mulpin_module_verify_s i2s[I2S_VERIFY_COUNT_X][I2S_VERIFY_COUNT_Y] = {
	{
		{_BOTTOM_,  48,  6},//I2SDATA
		{_BOTTOM_,  49,  6},//I2SBCK
		{_BOTTOM_,  50,  6},//I2SLRCK
		{_RIGHT_,   1,   6},//I2SMCK
	},
	{
		{_RIGHT_,   13,  3},//I2SDATA
		{_RIGHT_,   14,  3},//I2SBCK
		{_RIGHT_,   15,  3},//I2SLRCK
		{_RIGHT_,   16,  3},//I2SMCK
	},
	{
		{_BOTTOM_,  22,  0},//I2SDATA
		{_BOTTOM_,  23,  0},//I2SBCK
		{_BOTTOM_,  24,  0},//I2SLRCK
		{_BOTTOM_,  25,  0},//I2SMCK
	}
};

#define UART2_VERIFY_COUNT_X 3
#define UART2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s uart2[UART2_VERIFY_COUNT_X][UART2_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  17,  3},//UART2TX
		{_RIGHT_,  18,  3},//UART2RX
	},
	{
		{_LEFT_,   16,  0},//UART2TX
		{_LEFT_,   17,  0},//UART2RX
	},
	{
		{_BOTTOM_, 4,   2},//UART2TX
		{_BOTTOM_, 5,   2},//UART2RX
	}
};

#define I2C2_VERIFY_COUNT_X 4
#define I2C2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2c2[I2C2_VERIFY_COUNT_X][I2C2_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  17,  5},//SDA2
		{_RIGHT_,  18,  5},//SCL2
	},
	{
		{_RIGHT_,  37,  0},//SDA2
		{_RIGHT_,  38,  0},//SCL2
	},
	{
		{_BOTTOM_, 4,   3},//SDA2
		{_BOTTOM_, 5,   3},//SCL2
	},
	{
		{_BOTTOM_, 26,  2},//SDA2
		{_BOTTOM_, 27,  2},//SCL2
	}
};

#define I2CDDC_VERIFY_COUNT_X 2
#define I2CDDC_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2cddc[I2CDDC_VERIFY_COUNT_X][I2CDDC_VERIFY_COUNT_Y] = {
	{
		{_TOP_,   22,  1},//DDCSDA
		{_TOP_,   23,  1},//DDCSCL
	},
	{
		{_LEFT_,  7,   0},//DDCSDA
		{_LEFT_,  8,   0},//DDCSCL
	}
};

#define AGC_VERIFY_COUNT_X 2
#define AGC_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s agc[AGC_VERIFY_COUNT_X][AGC_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  39,  0},//AGC
	},
	{
		{_RIGHT_,  46,  0},//AGC
	},
};

static int module_funs_select_compare(struct mulpin_module_verify_s* p)
{
	int i = 0;
	for(i = 0; i < array_count; i++)
	{
		if((p->num == mulpin_table[i].chip_core_num) && (p->side == mulpin_table[i].side))
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
	//sdbg verify
	if (mulpin_verify(SDBG_VERIFY_COUNT_X, SDBG_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)sdbg)) {
		ret = -1;
		printf("sdbg verify end.\n");
	}
#endif

	//smartcard verify
	if (mulpin_verify(SMARTCARD_VERIFY_COUNT_X, SMARTCARD_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)smartcard)) {
		ret = -1;
		printf("smartcard verify end.\n");
	}

	//i2s verify
	if (mulpin_verify(I2S_VERIFY_COUNT_X, I2S_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2s)) {
		ret = -1;
		printf("i2s verify end.\n");
	}

	//uart2 verify
	if (mulpin_verify(UART2_VERIFY_COUNT_X, UART2_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)uart2)) {
		ret = -1;
		printf("uart2 verify end.\n");
	}

	//i2c2 verify
	if (mulpin_verify(I2C2_VERIFY_COUNT_X, I2C2_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2c2)) {
		ret = -1;
		printf("i2c2 verify end.\n");
	}

	//i2cddc verify
	if (mulpin_verify(I2CDDC_VERIFY_COUNT_X, I2CDDC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)i2cddc)) {
		ret = -1;
		printf("i2cddc verify end.\n");
	}

	//agc verify
	if (mulpin_verify(AGC_VERIFY_COUNT_X, AGC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)agc)) {
		ret = -1;
		printf("agc verify end.\n");
	}

	return ret;
}

void mulpin_init(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if((mulpin_table[i].chip_core_num & mulpin_table[i].sel0 \
			& mulpin_table[i].sel1 & mulpin_table[i].sel2 &  mulpin_table[i].fun) == ARRAY_END_FLAG) {
			array_count = i;
			break;
		}
	}
	for(i = 0; i < array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){

			if(mulpin_table[i].sel0 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel0, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel0 - 32, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel0 - 64, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel0 - 96, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 160){
				REG_SET_CLR_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel0 - 128, mulpin_table[i].fun & 0x1);
			}
			if(mulpin_table[i].sel1 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel1, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel1 - 32, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel1 - 64, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel1 - 96, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 160){
				REG_SET_CLR_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel1 - 128, (mulpin_table[i].fun & 0x2) >> 1);
			}
			if(mulpin_table[i].sel2 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel2, (mulpin_table[i].fun & 0x4) >> 2);
			}else if (mulpin_table[i].sel2 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel2 - 32, (mulpin_table[i].fun & 0x4) >> 2);
			}else if (mulpin_table[i].sel2 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel2 - 64, (mulpin_table[i].fun & 0x4) >> 2);
			}else if (mulpin_table[i].sel2 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel2 - 96, (mulpin_table[i].fun & 0x4) >> 2);
			}else if (mulpin_table[i].sel2 < 160){
				REG_SET_CLR_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel2 - 128, (mulpin_table[i].fun & 0x4) >> 2);
			}

		}
	}
/*	printf("REG_PINMUX_PORTA=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTA);
	printf("REG_PINMUX_PORTB=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTB);
	printf("REG_PINMUX_PORTC=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTC);
	printf("REG_PINMUX_PORTD=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTD);*/

	/* cmp the config fun & actual fun */
	unsigned char b0, b1, b2, act_fun;
	for(i = 0; i < array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){
			act_fun = b0 = b1 = b2 = 0;
			if(mulpin_table[i].sel0 < 32){
				b0 = REG_GET_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel0);
			}else if (mulpin_table[i].sel0 < 64){
				b0 = REG_GET_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel0 - 32);
			}else if (mulpin_table[i].sel0 < 96){
				b0 = REG_GET_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel0 - 64);
			}else if (mulpin_table[i].sel0 < 128){
				b0 = REG_GET_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel0 - 96);
			}else if (mulpin_table[i].sel0 < 160){
				b0 = REG_GET_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel0 - 128);
			}
			if(mulpin_table[i].sel1 < 32){
				b1 = REG_GET_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel1);
			}else if (mulpin_table[i].sel1 < 64){
				b1 = REG_GET_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel1 - 32);
			}else if (mulpin_table[i].sel1 < 96){
				b1 = REG_GET_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel1 - 64);
			}else if (mulpin_table[i].sel1 < 128){
				b1 = REG_GET_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel1 - 96);
			}else if (mulpin_table[i].sel1 < 160){
				b1 = REG_GET_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel1 - 128);
			}
			if(mulpin_table[i].sel2 < 32){
				b2 = REG_GET_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel2);
			}else if (mulpin_table[i].sel2 < 64){
				b2 = REG_GET_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel2 - 32);
			}else if (mulpin_table[i].sel2 < 96){
				b2 = REG_GET_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel2 - 64);
			}else if (mulpin_table[i].sel2 < 128){
				b2 = REG_GET_BIT(REG_PINMUX_PORTD, mulpin_table[i].sel2 - 96);
			}else if (mulpin_table[i].sel2 < 160){
				b2 = REG_GET_BIT(REG_PINMUX_PORTE, mulpin_table[i].sel2 - 128);
			}
			if(mulpin_table[i].sel1 == MULPIN_INVALID_VALUE)
				b1 = 0;
			if(mulpin_table[i].sel2 == MULPIN_INVALID_VALUE)
				b2 = 0;

			act_fun = (b2 << 2) + (b1 << 1) + b0;
			if(act_fun != mulpin_table[i].fun){
				printf("mulpin config error:(chip_core_num=%d)config fun=%d, actual fun=%d.\n", \
						mulpin_table[i].chip_core_num, mulpin_table[i].fun, act_fun);
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
	unsigned int i 				= 0;
	unsigned int offs 			= 0;		/* 0~31 */
	unsigned int entry_num			= 0;
	unsigned int gx_gpio_register = 0;
	struct gpio_table_header* sram_gpio_table_header = (struct gpio_table_header*)(GPIO_TABLE_START_ADDR);
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
		} else
			printf("file(%s) line(%d):gpio parameters error.\n", __FILE__, __LINE__);

		if (g_gpio_table[i].io_mode) {
			*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_OUT) |= 1 << offs;
			if (g_gpio_table[i].output_value)
				*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_HI) = 1 << offs;
			else
				*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_LO) = 1 << offs;
		}  else
			*(volatile unsigned int *)(gx_gpio_register + GPIO_SET_IN) = 1 << offs;
	}
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<23;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<26);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<26);
			}
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4800) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4800) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<8);
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
			/* serial ts clk select */
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<24;
			/* edge select */
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<27);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<27);
			}
			/* serial ts select input(clk/sync/valid/dat) */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4804) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4804) |= pin_config;
			/* serial ts enable */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<9);
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<25;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<28);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<28);
			}
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<10);
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
	} else{
		printf("ts_port=%d not support, please input 1/2/3.\n", ts_port);
		return -1;
	}

	return 0;
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

void dac_gain_control(void)
{
#ifdef CONFIG_ENABLE_GX_OTP
	int i = 0;
	unsigned char otp_data[4] = {0x0};
	unsigned char default_data[4] = {0x1e, 0x1e, 0x1e, 0x20};
	unsigned int otp_addr[4] = {0x126, 0x127, 0x128, 0x129};
	for (i = 0; i < 4; i++) {
		gx_otp_read(otp_addr[i], 1, &otp_data[i]);
		if (otp_data[i] != 0)
			otp_data[i] = otp_data[i] & 0x3f;
		else {
			otp_data[i] = default_data[i];
			printf("\nwarning : use default dac gain = 0x%x !\n", otp_data[i]);
		}
	}
	*(volatile unsigned int*)(VPU_BASE_ADDR + 0x130) =
		((otp_data[3] << 24) | (otp_data[2] << 16) | (otp_data[1] << 8) | (otp_data[0]));
#else
	printf("\n\nerror : configure DAC must set ENABLE_GX_OTP = y !\n\n");
	while(1);
#endif
}


extern enum vout_dac_case default_dac_case_vpu;
extern enum vout_dac_case default_dac_case_svpu;
void vout_init(void)
{

	/* set svpu dac case */
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) &= ~(0x1f<<25);
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x00) |=  (default_dac_case_svpu<<25);
	/* set vpu dac case */
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x00) &= ~(0x1f<<25);
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x00) |=  (default_dac_case_vpu<<25);

	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x70) &= 0xff00ffff;
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR + 0x70) |= 89 << 16;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x138)      = 0x1;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x140)      = 0;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x144)      = 0;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x148)      = 256 << 8;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x14c)      = 0;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x4164)    &= 0xffffff00;
	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x4168)    &= 0xffffff00;

	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x0c)  = ((80 << 13) | (80 + 720));
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR  + 0x1b0) = 0x163863;
	*(unsigned int volatile *)(VPU_VOUT_BASE_ADDR  + 0x1b8) = 0x163863;
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x110) = 0x10000001;
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x130) = 0x10000001;
	*(unsigned int volatile *)(SVPU_VOUT_BASE_ADDR + 0x150) = 0x10000001;

	//video dac
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x1<<0);  //enextref set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) |= (0x1<<1);   //envbg set high
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x7<<4);  //enctr set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0xf<<8);  //ensc set zero

	dac_gain_control(); // set video dac gain
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

#ifndef CONFIG_EXTERN_PARAM
/* denali_config.c / h5tq2g63bfr_h9.c */
unsigned int DENALI_CONFIG_CTL[155] __attribute__((section(".sram"))) = {
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
	DENALI_CTL_154_DATA
};

unsigned int DENALI_CONFIG_PHY[26]  __attribute__((section(".sram"))) = {
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
};
#endif

