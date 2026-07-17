#include "board-common.h"
#include "board-scorpio.h"
#include "cpu_config.h"
#include "driver/gx_padmux.h"
#include "gxav_vpu_propertytypes.h"
#include DDR_PAR_INCLUDE

/* multipin & gpio config init */
#define ARRAY_MAX_COUNT	1000

extern struct scorpio_mulpin_config_s scorpio_mulpin_table[];
extern struct gpio_entry_bootloader g_gpio_table[];
static int array_count = 0;
struct mulpin_module_verify_s{
	unsigned short num;
	unsigned char fun;
};

#define WP_HOLD_VERIFY_COUNT_X 3
#define WP_HOLD_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s wp_hold[WP_HOLD_VERIFY_COUNT_X][WP_HOLD_VERIFY_COUNT_Y] = {
	{
		{4,   1},//WP
		{5,   1},//HOLD
	},
	{
		{6,   1},//WP
		{7,   1},//HOLD
	},
	{
		{17,  3},//WP
		{18,  4},//HOLD
	}
};

#define SMARTCARD_VERIFY_COUNT_X 2
#define SMARTCARD_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s smartcard[SMARTCARD_VERIFY_COUNT_X][SMARTCARD_VERIFY_COUNT_Y] = {
	{
		{8,   1},//SC1CLK
		{9,   1},//SC1RST
		{10,  1},//SC1PWD
		{11,  1},//SC1CD
		{12,  1},//SC1DATA
	},
	{
		{26,  1},//SC1CLK
		{27,  1},//SC1RST
		{28,  1},//SC1PWD
		{29,  1},//SC1CD
		{30,  1},//SC1DATA
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

#define UART2_VERIFY_COUNT_X 3
#define UART2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s uart2[UART2_VERIFY_COUNT_X][UART2_VERIFY_COUNT_Y] = {
	{
		{13,  2},//UART2TX
		{14,  2},//UART2RX
	},
	{
		{17,  2},//UART2TX
		{18,  2},//UART2RX
	},
	{
		{35,  1},//UART2TX
		{36,  1},//UART2RX
	}
};

#if 0
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
#endif

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

#define I2C2_VERIFY_COUNT_X 4
#define I2C2_VERIFY_COUNT_Y 2
static struct mulpin_module_verify_s i2c2[I2C2_VERIFY_COUNT_X][I2C2_VERIFY_COUNT_Y] = {
	{
		{15,  2},//SDA2
		{16,  2},//SCL2
	},
	{
		{17,  1},//SDA2
		{18,  1},//SCL2
	},
	{
		{31,  1},//SDA2
		{32,  1},//SCL2
	},
	{
		{48,  1},//SDA2
		{49,  1},//SCL2
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

#if 0
#define SDBG_VERIFY_COUNT_X 2
#define SDBG_VERIFY_COUNT_Y 5
static struct mulpin_module_verify_s sdbg[SDBG_VERIFY_COUNT_X][SDBG_VERIFY_COUNT_Y] = {
	{
		{20,  2},//SDBGTDI
		{21,  2},//SDBGTDO
		{22,  2},//SDBGTMS
		{23,  2},//SDBGTCK
		{24,  2},//SDBGTRST
	},
	{
		{26,  6},//SDBGTDI
		{27,  6},//SDBGTDO
		{28,  6},//SDBGTMS
		{29,  6},//SDBGTCK
		{30,  6},//SDBGTRST
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
#endif

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

#define RM_VERIFY_COUNT_X 2
#define RM_VERIFY_COUNT_Y 9
static struct mulpin_module_verify_s rm[RM_VERIFY_COUNT_X][RM_VERIFY_COUNT_Y] = {
	{
		{31,  4},//RMCRSDV
		{32,  4},//MD
		{34,  5},//MDC
		{35,  4},//RMTXEN
		{36,  4},//RMTXD1
		{37,  3},//RMTXD0
		{38,  3},//RMCLK
		{48,  3},//RMRXD1
		{49,  3},//RMRXD0
	},
	{
		{60,  2},//RMCRSDV
		{61,  2},//MD
		{62,  2},//MDC
		{63,  2},//RMTXEN
		{64,  2},//RMTXD1
		{65,  2},//RMTXD0
		{66,  2},//RMCLK
		{67,  2},//RMRXD1
		{68,  2},//RMRXD0
	}
};

static int module_funs_select_compare(struct mulpin_module_verify_s* p)
{
	int i = 0;
	for(i = 0; i < array_count; i++)
	{
		if(p->num == scorpio_mulpin_table[i].pin_id)
		{
			if(p->fun == scorpio_mulpin_table[i].fun)
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

#if 1
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

#if 0
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
#endif

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

void scorpio_mulpin_init(void)
{
	int i = 0;
	unsigned char act_fun;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if(scorpio_mulpin_table[i].pin_id == ARRAY_END_FLAG_U16) {
			array_count = i;
			break;
		}
	}

	for(i = 0; i < array_count; i++){
		if(scorpio_mulpin_table[i].fun != NOT_CONFIG)
			padmux_set(scorpio_mulpin_table[i].pin_id, scorpio_mulpin_table[i].fun);
	}

	/* cmp the config fun & actual fun */
	for(i = 0; i < array_count; i++){
		if(scorpio_mulpin_table[i].fun != NOT_CONFIG){
			act_fun = padmux_get(scorpio_mulpin_table[i].pin_id);
			if (act_fun != scorpio_mulpin_table[i].fun) {
				printf("mulpin config error:(pin_id=%d)config fun=%d, actual fun=%d.\n", \
						scorpio_mulpin_table[i].pin_id, scorpio_mulpin_table[i].fun, act_fun);
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

/*
 * Detailed explanation see the board-common.h
 * */
int scorpio_ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select)
{
#ifndef CONFIG_CLOCK_PIDFILTER_DISABLE
	if (port_mode)
		*(volatile unsigned int*)(REG_BASE_PIDFILTER + 0x10d0) = pin_config;
#endif
	if(ts_port == 0){
		if(port_mode){
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= 1<<11;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) &= ~(1<<15);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= (1<<15);
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= 1<<12;
			/* edge select */
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) &= ~(1<<16);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= (1<<16);
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= 1<<13;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) &= ~(1<<17);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= (1<<17);
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
	} else if(ts_port == 3){
		if(port_mode){
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= 1<<14;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) &= ~(1<<18);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE2_SEL) |= (1<<18);
			}
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x4808) |= pin_config;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x36d4) |= (1<<11);
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
	} else{
		printf("ts_port=%d not support, please input 1/2/3.\n", ts_port);
		return -1;
	}

	return 0;
}

void sci_config(int vccen_pol, int detect_pol)
{
	*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_CONFIG2_REG_1CLR) = 1 << 11;
	*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_CLEAR3) = 1 << 16;

	unsigned int val = *(volatile unsigned int *)(REG_BASE_SMARTCARD + 0x4);

	if (vccen_pol)
		val &= ~(1<<13);
	else
		val |= 1<<13;

	if (detect_pol)
		val &= ~(1<<0);
	else
		val |= 1<<0;

	*(volatile unsigned int *)(REG_BASE_SMARTCARD + 0x4) = val;

	*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_CONFIG2_REG_1SET) = 1 << 11;
	*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_SET3) = 1 << 16;
}

void scorpio_dac_gain_control(void)
{
#ifdef CONFIG_ENABLE_GX_OTP
	unsigned char otp_data = 0x0;
	gx_otp_read(0x128, 1, &otp_data);
	if (otp_data != 0) {
		otp_data = otp_data & 0x3f;
	} else {
		otp_data = 0x20;
		printf("\nwarning : use default dac gain = 0x%x !\n", 0x20202020);
	}
	vpu_sys_set_dac_gain(1 << 0, otp_data);
	vpu_sys_set_dac_gain(1 << 1, otp_data);
	vpu_sys_set_dac_gain(1 << 2, otp_data);
	vpu_sys_set_dac_gain(1 << 3, otp_data);
#else
	printf("\n\nerror : configure DAC must set ENABLE_GX_OTP = y !\n\n");
	while(1);
#endif
}


extern enum vout_dac_case default_dac_case_vpu;
extern enum vout_dac_case default_dac_case_svpu;
void scorpio_vout_init(void)
{
	unsigned int vout_addr[2] = {0};
	unsigned short fir[10] = {0};

	vpu_setup();
	clock_setup();
	vout_addr[0] = VPU_VOUT_BASE_ADDR;
	vout_addr[1] = VPU_VOUT_BASE_ADDR + 0x1000;
	vout_install(vout_addr);
	vpu_install(VPU_BASE_ADDR);
	clock_set_vpu_enable(1);
	clock_set_svpu_enable(1);
	clock_set_hdmi_enable(1);
	clock_set_vdac_enable(1);
	clock_set_jpeg_enable(1);
#ifndef CONFIG_FPGA_BOARD
	//set vdac
	vpu_sys_set_dac_en(1 << 0, 1);
	vpu_sys_set_dac(1 << 0, SD_MIXER);
	/* set svpu dac case */
	vout_set_dac_fullcase_change(1, default_dac_case_svpu);
	/* set vpu dac case */
	vout_set_dac_fullcase_change(0, default_dac_case_svpu);
	vout_set_bt656_hpos(0, 89);
	vout_set_dac_mode(0, CASE_GBRX);
	vout_set_dac_mode(1, CASE_GBRX);
	fir[9]= 256;
	vpu_vpp_set_hfilter(0, 1, fir);
	vout_set_tv_active(1, 80, 800);
	vout_set_mix_fir_para(0, 0x163863, 0x163863);
	vout_set_gamma_ctrl(1, 0x10000001, 0x10000001, 0x10000001);
	//video dac
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x1<<0);  //enextref set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) |= (0x1<<1);   //envbg set high
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x7<<4);  //enctr set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0xf<<8);  //ensc set zero
	
	scorpio_dac_gain_control();  // set video dac gain
	return;
#endif
}

