#include "board-common.h"
#include "board-taurus.h"
#include "cpu_config.h"
#include "gxav_vpu_propertytypes.h"
#include DDR_PAR_INCLUDE

/* multipin & gpio config init */
#define ARRAY_MAX_COUNT	1000

extern struct taurus_mulpin_config_s taurus_mulpin_table[];
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
		{_BOTTOM_,  53,  3},//SDBGTDI
		{_BOTTOM_,  54,  3},//SDBGTDO
		{_BOTTOM_,  55,  3},//SDBGTMS
		{_BOTTOM_,  56,  2},//SDBGTCK
		{_BOTTOM_,  57,  2},//SDBGTRST
	},
	{
		{_RIGHT_,   5,   3},//SDBGTDI
		{_RIGHT_,   6,   3},//SDBGTDO
		{_RIGHT_,   7,   3},//SDBGTMS
		{_RIGHT_,   8,   3},//SDBGTCK
		{_RIGHT_,   9,   3},//SDBGTRST
	},
};
#endif

#define HVSEL_VERIFY_COUNT_X 2
#define HVSEL_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s hvsel[HVSEL_VERIFY_COUNT_X][HVSEL_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  14,  0},//HVSEL
	},
	{
		{_RIGHT_,  42,  0},//HVSEL
	},
};

#define DISEQCO_VERIFY_COUNT_X 2
#define DISEQCO_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s diseqco[DISEQCO_VERIFY_COUNT_X][DISEQCO_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  15,  0},//DiSEqCo
	},
	{
		{_RIGHT_,  43,  0},//DiSEqCo
	},
};

#define AGC_VERIFY_COUNT_X 2
#define AGC_VERIFY_COUNT_Y 1
static struct mulpin_module_verify_s agc[AGC_VERIFY_COUNT_X][AGC_VERIFY_COUNT_Y] = {
	{
		{_RIGHT_,  16,  0},//AGC
	},
	{
		{_RIGHT_,  44,  0},//AGC
	},
};

static int module_funs_select_compare(struct mulpin_module_verify_s* p)
{
	int i = 0;
	for(i = 0; i < array_count; i++)
	{
		if((p->num == taurus_mulpin_table[i].chip_core_num) && (p->side == taurus_mulpin_table[i].side))
		{
			if(p->fun == taurus_mulpin_table[i].fun)
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

	//hvsel verify
	if (mulpin_verify(HVSEL_VERIFY_COUNT_X, HVSEL_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)hvsel)) {
		ret = -1;
		printf("hvsel verify end.\n");
	}

	//diseqco verify
	if (mulpin_verify(DISEQCO_VERIFY_COUNT_X, DISEQCO_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)diseqco)) {
		ret = -1;
		printf("diseqco verify end.\n");
	}

	//agc verify
	if (mulpin_verify(AGC_VERIFY_COUNT_X, AGC_VERIFY_COUNT_Y, (struct mulpin_module_verify_s *)agc)) {
		ret = -1;
		printf("agc verify end.\n");
	}

	return ret;
}

void taurus_mulpin_init(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if((taurus_mulpin_table[i].chip_core_num & taurus_mulpin_table[i].sel0 \
			& taurus_mulpin_table[i].sel1 & taurus_mulpin_table[i].sel2 &  taurus_mulpin_table[i].fun) == ARRAY_END_FLAG) {
			array_count = i;
			break;
		}
	}
	for(i = 0; i < array_count; i++){
		if(taurus_mulpin_table[i].fun != NOT_CONFIG){

			if(taurus_mulpin_table[i].sel0 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel0, taurus_mulpin_table[i].fun & 0x1);
			}else if (taurus_mulpin_table[i].sel0 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel0 - 32, taurus_mulpin_table[i].fun & 0x1);
			}else if (taurus_mulpin_table[i].sel0 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel0 - 64, taurus_mulpin_table[i].fun & 0x1);
			}else if (taurus_mulpin_table[i].sel0 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel0 - 96, taurus_mulpin_table[i].fun & 0x1);
			}
			if(taurus_mulpin_table[i].sel1 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel1, (taurus_mulpin_table[i].fun & 0x2) >> 1);
			}else if (taurus_mulpin_table[i].sel1 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel1 - 32, (taurus_mulpin_table[i].fun & 0x2) >> 1);
			}else if (taurus_mulpin_table[i].sel1 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel1 - 64, (taurus_mulpin_table[i].fun & 0x2) >> 1);
			}else if (taurus_mulpin_table[i].sel1 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel1 - 96, (taurus_mulpin_table[i].fun & 0x2) >> 1);
			}
			if(taurus_mulpin_table[i].sel2 < 32){
				REG_SET_CLR_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel2, (taurus_mulpin_table[i].fun & 0x4) >> 2);
			}else if (taurus_mulpin_table[i].sel2 < 64){
				REG_SET_CLR_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel2 - 32, (taurus_mulpin_table[i].fun & 0x4) >> 2);
			}else if (taurus_mulpin_table[i].sel2 < 96){
				REG_SET_CLR_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel2 - 64, (taurus_mulpin_table[i].fun & 0x4) >> 2);
			}else if (taurus_mulpin_table[i].sel2 < 128){
				REG_SET_CLR_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel2 - 96, (taurus_mulpin_table[i].fun & 0x4) >> 2);
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
		if(taurus_mulpin_table[i].fun != NOT_CONFIG){
			act_fun = b0 = b1 = b2 = 0;
			if(taurus_mulpin_table[i].sel0 < 32){
				b0 = REG_GET_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel0);
			}else if (taurus_mulpin_table[i].sel0 < 64){
				b0 = REG_GET_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel0 - 32);
			}else if (taurus_mulpin_table[i].sel0 < 96){
				b0 = REG_GET_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel0 - 64);
			}else if (taurus_mulpin_table[i].sel0 < 128){
				b0 = REG_GET_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel0 - 96);
			}
			if(taurus_mulpin_table[i].sel1 < 32){
				b1 = REG_GET_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel1);
			}else if (taurus_mulpin_table[i].sel1 < 64){
				b1 = REG_GET_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel1 - 32);
			}else if (taurus_mulpin_table[i].sel1 < 96){
				b1 = REG_GET_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel1 - 64);
			}else if (taurus_mulpin_table[i].sel1 < 128){
				b1 = REG_GET_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel1 - 96);
			}
			if(taurus_mulpin_table[i].sel2 < 32){
				b2 = REG_GET_BIT(REG_PINMUX_PORTA, taurus_mulpin_table[i].sel2);
			}else if (taurus_mulpin_table[i].sel2 < 64){
				b2 = REG_GET_BIT(REG_PINMUX_PORTB, taurus_mulpin_table[i].sel2 - 32);
			}else if (taurus_mulpin_table[i].sel2 < 96){
				b2 = REG_GET_BIT(REG_PINMUX_PORTC, taurus_mulpin_table[i].sel2 - 64);
			}else if (taurus_mulpin_table[i].sel2 < 128){
				b2 = REG_GET_BIT(REG_PINMUX_PORTD, taurus_mulpin_table[i].sel2 - 96);
			}
			if(taurus_mulpin_table[i].sel1 == MULPIN_INVALID_VALUE)
				b1 = 0;
			if(taurus_mulpin_table[i].sel2 == MULPIN_INVALID_VALUE)
				b2 = 0;

			act_fun = (b2 << 2) + (b1 << 1) + b0;
			if(act_fun != taurus_mulpin_table[i].fun){
				printf("mulpin config error:(chip_core_num=%d)config fun=%d, actual fun=%d.\n", \
						taurus_mulpin_table[i].chip_core_num, taurus_mulpin_table[i].fun, act_fun);
			}
		}
	}

#ifdef CONFIG_MULPIN_VERIFY
	/*module mulpin conflict verify*/
	if(mulpin_module_verify() != 0)
	{
		printf("please resolve the taurus_mulpin_table conflict error.\n");
		while(1);
	}
#endif
}

/*
 * Detailed explanation see the board-common.h
 * */
int taurus_ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select)
{
#ifndef CONFIG_CLOCK_PIDFILTER_DISABLE
	if (port_mode)
		*(volatile unsigned int*)(REG_BASE_PIDFILTER + 0x10d0) = pin_config;
#endif
	if(ts_port == 1){
		if(port_mode){
			/* serial ts clk select */
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<12;
			/* edge select */
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<16);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<16);
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<13;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<17);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<17);
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
			*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= 1<<14;
			if(edge_select == 1){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) &= ~(1<<18);
			}else if(edge_select == 0){
				*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL2) |= (1<<18);
			}
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x480C) &= ~0xFFFFFFFF;
			*(volatile unsigned int*)(REG_BASE_DEMUX + 0x480C) |= pin_config;
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
	} else{
		printf("ts_port=%d not support, please input 1/2/3.\n", ts_port);
		return -1;
	}

	return 0;
}

void taurus_dac_gain_control(void)
{
#ifdef CONFIG_ENABLE_GX_OTP
	unsigned char otp_data = 0x0;
	gx_otp_read(0x128, 1, &otp_data);
	if (otp_data != 0) {
		otp_data = otp_data & 0x3f;
		vpu_sys_set_dac_gain(1 << 0, otp_data);
	} else {
		vpu_sys_set_dac_gain(1 << 0, 0x20);
		printf("\nwarning : use default dac gain = 0x%x !\n", 0x20202020);
	}
#else
	printf("\n\nerror : configure DAC must set ENABLE_GX_OTP = y !\n\n");
	while(1);
#endif
	return;
}

static void _tmp_config(void)
{
//	*(unsigned int volatile *)(VPU_BASE_ADDR + 0x138)      = 0x1;
}

void taurus_vout_init(void)
{
	unsigned int vout_addr[2] = {0};
	unsigned short fir[10] = {0};

	vpu_setup();
	clock_setup();
	vout_addr[0] = TAURUSVPU_VOUT_BASE_ADDR;
	vout_addr[1] = TAURUSSVPU_VOUT_BASE_ADDR;
	vout_install(vout_addr);
	vpu_install(VPU_BASE_ADDR);
	clock_set_jpeg_enable(1);
	clock_set_vpu_enable(1);
#ifndef CONFIG_FPGA_BOARD
	//set vdac
	clock_set_svpu_enable(1);
	vpu_sys_set_dac_en(1 << 0, 1);
	vpu_sys_set_dac(1 << 0, SD_MIXER);
	/* set svpu dac case */
	vout_set_dac_fullcase_change(1, CASE_GBRX);
	/* set vpu dac case */
	vout_set_dac_fullcase_change(0, CASE_BGRX);
	vout_set_bt656_hpos(0, 89);
	vout_set_dac_mode(0, CASE_GBRX);
	vout_set_dac_mode(1, CASE_GBRX);
	fir[9]= 256;
	vpu_vpp_set_hfilter(0, 1, fir);
	vout_set_tv_active(1, 80, 800);
	vout_set_mix_fir_para(0, 0x163863, 0x163863);
	vpu_ehc_set_green_blue(0, 0);
	vout_set_gamma_ctrl(1, 0x10000001, 0x10000001, 0x10000001);
	_tmp_config();
	//video dac
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x1<<0);  //enextref set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) |= (0x1<<1);   //envbg set high
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0x7<<4);  //enctr set zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_DAC_CONFIG_0) &= ~(0xf<<8);  //ensc set zero
	taurus_dac_gain_control();  // set video dac gain
#endif

	return;
}


