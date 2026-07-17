#include "board-common.h"
#include "cpu_config.h"
#include "interrupt.h"

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
		break;
	case TS_MUX_TS_S_0:
		if (ts_in != TS_IN_TS0_S) {
			printf("TS_MUX_TS_S_1 only support TS_IN_TS0_S\n");
			return -1;
		}
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<15);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<15);
		break;
	case TS_MUX_TS_P_1:
		if (ts_in == TS_IN_TS3_P_S) {
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<2);
			*(volatile unsigned int*)(REG_BASE_PIN_CONFIG + 0x100) |= (0x1<<27);
		} else if (ts_in == TS_IN_TS1_P_S) {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4848) &= ~0x1;
		} else {
			printf("TS_MUX_TS_P_1 only support TS_IN_TS1_P_S and TS_IN_TS3_P_S\n");
			return -1;
		}
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<16);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<16);
		break;
	case TS_MUX_TS_P_2:
		if (ts_in == TS_IN_TS2_P_S) {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4848) &= ~(0x1 << 1);
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<1);
		} else if (ts_in == TS_IN_TS3_P_S) {
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) &= ~(0x1<<2);
			*(volatile unsigned int*)(REG_BASE_PIN_CONFIG + 0x100) &= ~(0x1<<27);
		} else {
			printf("TS_MUX_TS_P_2 only support TS_IN_TS3_P\n");
			return -1;
		}
		if (edge_select == 1)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) &= ~(1<<17);
		else if (edge_select == 0)
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_SOURCE2) |= (1<<17);
		break;
	case TS_MUX_DVB_TS_OUT1:
		if (ts_in == TS_IN_TS2_P_S) {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4848) &= ~(0x1 << 1);
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) |= (0x1<<1);
		} else if (ts_in == TS_IN_TS3_P_S)
			*(volatile unsigned int*)(REG_BASE_CHIPCONFIG + 0x700) |= (0x1<<2);
		else {
			printf("TS_MUX_DVB_TS_OUT1 only support TS_IN_TS2_P_S and TS_IN_TS3_P_S\n");
			return -1;
		}
		break;
	case TS_MUX_TSIO0:
		if (ts_in == TS_IN_TS1_P_S) {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4848) |= 0x1;
		} else {
			printf("TS_MUX_TSIO0 only support TS_IN_TS1_P_S\n");
			return -1;
		}
		break;
	case TS_MUX_TSIO1:
		if (ts_in == TS_IN_TS2_P_S) {
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4848) |= (0x1 << 1);
		} else {
			printf("TS_MUX_TSIO0 only support TS_IN_TS1_P_S\n");
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
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x1480C) = *(volatile unsigned int*)(REG_BASE_DEMUX + 0x480C);
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
	unsigned int otp_addr[2] = {0x266, 0x267};

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
#define VEGA_HDMI_BASE_ADDR (0x8ae00000)
void vout_init(void)
{
	int default_dac_mode_svpu = 0;

#ifdef CONFIG_ARCH_ARM_VEGA
	// set vdac
	*(unsigned int volatile *)(VDAC_BASE_ADDR + (0x00<<2)) = 0;
	*(unsigned int volatile *)(VDAC_BASE_ADDR + (0x00<<2)) = 0xC0;
	*(unsigned int volatile *)(VEGA_HDMI_BASE_ADDR + (0x4001<<2)) |= 0x40;
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
