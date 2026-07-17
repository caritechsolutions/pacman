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
static int mulpin_array_count = 0;

struct mulpin_module_verify_s{
	unsigned char side;
	unsigned char num;
	unsigned char fun;
};

#define SMARTCARD_VERIFY_COUNT	5
static struct mulpin_module_verify_s smardcard[3][SMARTCARD_VERIFY_COUNT] = {
	{
		{_BOTTOM_, 74, 0},	//SC1CLK
		{_BOTTOM_, 75, 0},	//SC1RST
		{_BOTTOM_, 76, 0},	//SC1PWR
		{_BOTTOM_, 77, 0},	//SC1CD
		{_BOTTOM_, 78, 0},	//SC1DATA
	},
	{
		{ _RIGHT_, 62, 2},	//SC1CLK
		{ _RIGHT_, 63, 2},	//SC1RST
		{ _RIGHT_, 64, 2},	//SC1PWR
		{ _RIGHT_, 65, 2},	//SC1CD
		{ _RIGHT_, 66, 2},	//SC1DATA
	},
	{
		{  _LEFT_,  2, 1},	//SC1CLK
		{  _LEFT_,  3, 1},	//SC1RST
		{  _LEFT_,  4, 1},	//SC1PWR
		{  _LEFT_,  5, 1},	//SC1CD
		{  _LEFT_,  6, 1},	//SC1DATA
	}
};

#define TSI2_VERIFY_COUNT	4
static struct mulpin_module_verify_s tsi2[2][TSI2_VERIFY_COUNT] = {
	{
		{ _RIGHT_, 39, 0},	//TSI2DATA5
		{ _RIGHT_, 40, 0},	//TSI2DATA6
		{ _RIGHT_, 41, 0},	//TSI2DATA7
		{ _RIGHT_, 42, 0},	//TSI2CLK
	},
	{
		{ _RIGHT_, 46, 2},	//TSI2DATA5
		{ _RIGHT_, 47, 2},	//TSI2DATA6
		{ _RIGHT_, 48, 2},	//TSI2DATA7
		{ _RIGHT_, 49, 2},	//TSI2CLK
	}
};

#define NETWORK_VERIFY_COUNT	9
static struct mulpin_module_verify_s network[2][NETWORK_VERIFY_COUNT] = {
	{
		{ _RIGHT_, 22, 0},	//RMCRSDV
		{ _RIGHT_, 23, 0},	//MD
		{ _RIGHT_, 24, 0},	//MDC
		{ _RIGHT_, 25, 0},	//RMTXEN
		{ _RIGHT_, 26, 0},	//RMTXD1
		{ _RIGHT_, 27, 0},	//RMTXD0
		{ _RIGHT_, 29, 0},	//RMCLK
		{ _RIGHT_, 30, 0},	//RMRXD1
		{ _RIGHT_, 31, 0},	//RMRXD0
	},
	{
		{   _TOP_, 84, 2},	//RMCRSDV
		{   _TOP_, 85, 2},	//MD
		{   _TOP_, 86, 2},	//MDC
		{  _LEFT_,  1, 2},	//RMTXEN
		{  _LEFT_,  2, 2},	//RMTXD1
		{  _LEFT_,  3, 2},	//RMTXD0
		{  _LEFT_,  4, 2},	//RMCLK
		{  _LEFT_,  5, 2},	//RMRXD1
		{  _LEFT_,  6, 2},	//RMRXD0
	}
};

#define NANDFLASH_VERIFY_COUNT	15
static struct mulpin_module_verify_s nandflash[2][NANDFLASH_VERIFY_COUNT] = {
	{
		{_BOTTOM_,101, 2},	//NFRDY1
		{_BOTTOM_,104, 3},	//NFCLE
		{ _RIGHT_,  1, 3},	//NFALE
		{ _RIGHT_,  3, 3},	//NFWE
		{ _RIGHT_,  5, 2},	//NFCS0n
		{ _RIGHT_, 62, 3},	//NFDAT7
		{ _RIGHT_, 63, 3},	//NFDAT6
		{ _RIGHT_, 64, 3},	//NFDAT5
		{ _RIGHT_, 65, 3},	//NFDAT4
		{ _RIGHT_, 66, 3},	//NFDAT3
		{ _RIGHT_, 72, 2},	//NFRDY0
		{ _RIGHT_, 73, 2},	//NFCS1n
		{ _RIGHT_, 74, 2},	//NFDAT2
		{ _RIGHT_, 75, 2},	//NFDAT1
		{ _RIGHT_, 76, 2},	//NFDAT0
	},
	{
		{   _TOP_, 85, 3},	//NFRDY1
		{  _LEFT_, 19, 0},	//NFCLE
		{  _LEFT_, 18, 0},	//NFALE
		{  _LEFT_, 16, 0},	//NFWE
		{   _TOP_, 86, 0},	//NFCS0n
		{  _LEFT_, 15, 0},	//NFDAT7
		{  _LEFT_, 14, 0},	//NFDAT6
		{  _LEFT_, 13, 0},	//NFDAT5
		{  _LEFT_, 12, 0},	//NFDAT4
		{  _LEFT_,  6, 0},	//NFDAT3
		{  _LEFT_, 21, 0},	//NFRDY0
		{  _LEFT_,  1, 0},	//NFCS1n
		{  _LEFT_,  5, 0},	//NFDAT2
		{  _LEFT_,  4, 0},	//NFDAT1
		{  _LEFT_,  3, 0},	//NFDAT0
	}
};

#define AUARTTX_VERIFY_COUNT	1
static struct mulpin_module_verify_s auarttx[2][AUARTTX_VERIFY_COUNT] = {
	{
		{ _RIGHT_, 29, 2},	//AUARTTX
	},
	{
		{ _RIGHT_, 70, 2},	//AUARTTX
	}
};

#define AOAMCLK_VERIFY_COUNT	1
static struct mulpin_module_verify_s aoamclk[2][AOAMCLK_VERIFY_COUNT] = {
	{
		{ _RIGHT_, 27, 2},	//AOAMCLK
	},
	{
		{  _LEFT_, 19, 1},	//AOAMCLK
	}
};

static int module_func_select_compare(struct mulpin_module_verify_s *p)
{
	int i;
	for(i = 0; i < mulpin_array_count; i++){
		if((p->side == mulpin_table[i].side) && (p->num == mulpin_table[i].num)){
			if(p->fun == mulpin_table[i].fun)
				return 0;
			else
				return -1;
		}
	}
	return -1;
}

int mulpin_module_verify(void)
{
	int i;
	int ret = 0;

	//tsi2 verify
	for(i = 0; i < TSI2_VERIFY_COUNT; i++){
		if((module_func_select_compare(&tsi2[0][i]) == 0) && (module_func_select_compare(&tsi2[1][i]) == 0)){
			printf("TSI2 mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					tsi2[0][i].side, tsi2[0][i].num, tsi2[1][i].side, tsi2[1][i].num);
			ret = -1;
		}
	}

	//smartcard verify
	for(i = 0; i < SMARTCARD_VERIFY_COUNT; i++){
		if((module_func_select_compare(&smardcard[0][i]) == 0) && (module_func_select_compare(&smardcard[1][i]) == 0)){
			printf("SMARTCARD mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					smardcard[0][i].side, smardcard[0][i].num, smardcard[1][i].side, smardcard[1][i].num);
			ret = -1;
		}
		if((module_func_select_compare(&smardcard[0][i]) == 0) && (module_func_select_compare(&smardcard[2][i]) == 0)){
			printf("SMARTCARD mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					smardcard[0][i].side, smardcard[0][i].num, smardcard[2][i].side, smardcard[2][i].num);
			ret = -1;
		}
		if((module_func_select_compare(&smardcard[1][i]) == 0) && (module_func_select_compare(&smardcard[2][i]) == 0)){
			printf("SMARTCARD mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					smardcard[1][i].side, smardcard[1][i].num, smardcard[2][i].side, smardcard[2][i].num);
			ret = -1;
		}
	}

	//nandflash verify
	for(i = 0; i < NANDFLASH_VERIFY_COUNT; i++){
		if((module_func_select_compare(&nandflash[0][i]) == 0) && (module_func_select_compare(&nandflash[1][i]) == 0)){
			printf("NANDFLASH mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					nandflash[0][i].side, nandflash[0][i].num, nandflash[1][i].side, nandflash[1][i].num);
			ret = -1;
		}
	}

	//network verify
	for(i = 0; i < NETWORK_VERIFY_COUNT; i++){
		if((module_func_select_compare(&network[0][i]) == 0) && (module_func_select_compare(&network[1][i]) == 0)){
			printf("NETWORK mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					network[0][i].side, network[0][i].num, network[1][i].side, network[1][i].num);
			ret = -1;
		}
	}

	//auarttx verify
	for(i = 0; i < AUARTTX_VERIFY_COUNT; i++){
		if((module_func_select_compare(&auarttx[0][i]) == 0) && (module_func_select_compare(&auarttx[1][i]) == 0)){
			printf("AUARTTX mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					auarttx[0][i].side, auarttx[0][i].num, auarttx[1][i].side, auarttx[1][i].num);
			ret = -1;
		}
	}

	//aoamclk verify
	for(i = 0; i < AOAMCLK_VERIFY_COUNT; i++){
		if((module_func_select_compare(&aoamclk[0][i]) == 0) && (module_func_select_compare(&aoamclk[1][i]) == 0)){
			printf("AOAMCLK mulpin verify error:(side=%d num=%d) and (side=%d num=%d) is both enabled.\n", \
					aoamclk[0][i].side, aoamclk[0][i].num, aoamclk[1][i].side, aoamclk[1][i].num);
			ret = -1;
		}
	}

	return ret;
}

void mulpin_init(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if((mulpin_table[i].side & mulpin_table[i].num & mulpin_table[i].sel0 \
			& mulpin_table[i].sel1 & mulpin_table[i].fun) == ARRAY_END_FLAG) {
			mulpin_array_count = i;
			break;
		}
	}

	for(i = 0; i < mulpin_array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){

			if(mulpin_table[i].sel0 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel0, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel0 - 32, mulpin_table[i].fun & 0x1);
			}else if (mulpin_table[i].sel0 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel0 - 64, mulpin_table[i].fun & 0x1);
			}
			if(mulpin_table[i].sel1 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel1, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel1 - 32, (mulpin_table[i].fun & 0x2) >> 1);
			}else if (mulpin_table[i].sel1 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel1 - 64, (mulpin_table[i].fun & 0x2) >> 1);
			}
		}
	}

/*	printf("REG_PINMUX_PORTA=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTA);
	printf("REG_PINMUX_PORTB=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTB);
	printf("REG_PINMUX_PORTC=0x%x.\n", *(volatile unsigned int*)REG_PINMUX_PORTC);*/

	/* cmp the config fun & actual fun */
	unsigned char lb, hb, act_fun;
	int err_cnt = 0;
	for(i = 0; i < mulpin_array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){
			act_fun = lb = hb = 0;
			if(mulpin_table[i].sel0 < 32){
				lb = REG_GET_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel0);
			}else if (mulpin_table[i].sel0 < 64){
				lb = REG_GET_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel0 - 32);
			}else if (mulpin_table[i].sel0 < 96){
				lb = REG_GET_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel0 - 64);
			}
			if(mulpin_table[i].sel1 < 32){
				hb = REG_GET_BIT(REG_PINMUX_PORTA, mulpin_table[i].sel1);
			}else if (mulpin_table[i].sel1 < 64){
				hb = REG_GET_BIT(REG_PINMUX_PORTB, mulpin_table[i].sel1 - 32);
			}else if (mulpin_table[i].sel1 < 96){
				hb = REG_GET_BIT(REG_PINMUX_PORTC, mulpin_table[i].sel1 - 64);
			}
			if((mulpin_table[i].sel0 == MULPIN_INVALID_VALUE) && (hb == 1))
				lb = 1;
			if(mulpin_table[i].sel1 == MULPIN_INVALID_VALUE)
				hb = 0;
			act_fun = (hb << 1) + lb;
			if(act_fun != mulpin_table[i].fun){
				printf("mulpin config error:(side=%d num=%d)config fun=%d, actual fun=%d.\n", \
						mulpin_table[i].side, mulpin_table[i].num, mulpin_table[i].fun, act_fun);
				err_cnt++;
			}
		}
	}
	if(err_cnt){
		printf("please resolve the mulpin_table config error.\n");
		while(1);
	}

	/* module mulpin conflict verify */
	if(mulpin_module_verify() != 0){
		printf("please resolve the mulpin_table module conflict error.\n");
		while(1);
	}
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
			*(volatile unsigned int *)(gx_gpio_register + GPIO_EPDDR) |= 1 << offs;
			if (g_gpio_table[i].output_value)
				*(volatile unsigned int *)(gx_gpio_register + GPIO_EPBSET) = 1 << offs;
			else
				*(volatile unsigned int *)(gx_gpio_register + GPIO_EPBCLR) = 1 << offs;
		}
	}
}

/*
 * Detailed explanation see the board-common.h
 * */
int ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select)
{
	if(ts_port == 1){
		if(port_mode){
			/* serial ts clk select */
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<12;
			/* edge select */
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<14);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<14);
			}
			/* serial ts select input(clk/sync/valid/dat) */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(7<<20);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (pin_config<<20);
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
	}else if(ts_port == 2){
		if(port_mode){
			/* serial ts clk select */
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<13;
			/* edge select */
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<15);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<15);
			}
			/* serial ts select input(clk/sync/valid/dat) */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) &= ~(7<<24);
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (pin_config<<24);
			/* serial ts enable */
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<10);
		}else{
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
	}else{
		printf("ts_port=%d not support, please input 1/2.\n", ts_port);
		return -1;
	}
	return 0;
}

void enable_dac(enum video_dac_mode mode)
{
	switch (mode){
	case ENABLE_CVBS:
		*(volatile unsigned int *)0xa4900088 |= 1<<0; //set sel
		*(volatile unsigned int *)0xa4900084 = 0x11;
		break;
	case ENABLE_CVBS_AND_YPBPR:
		*(volatile unsigned int *)0xa4900088 |= 1<<0; //set sel
		*(volatile unsigned int *)0xa4900084 = 0x1f;
		break;
	default:
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

