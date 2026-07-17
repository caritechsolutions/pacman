static struct dto_param dto_table_full[] = {
	{1  , 1, 0x02a5de08 , 1 <<  1},		// Audio i2s		99MHz
	{2  , 1, 0x05555555 , 1 <<  2},		// Audio spdif		99MHz
	{3  , 1, 0x0aaaaaaa , 1 << 12},		// Video			198Mhz
	{4  , 1, 0x08000000 , 1 << 13},		// JPEG				148.5MHZ
	{5  , 1, 0x08000000 , 1 << 14},		// PP				148.5MHZ
	{6  , 1, 0x0ccccccc , 1 << 15},		// Audio decoder	237.6MHZ
	{7  , 1, 0x09245FD9 , 1 << 16},		// GA				169.714285714MHz
	{8  , 1, 0x09245FD9 , 1 << 17},		// DemuxSys			169.714285714MHz
	{9  , 1, 0x05D1745D , 1 << 18},		// DemuxStc			108MHz
	{14 , 2, 0x08000000 , 1 << 19},		// SCPU				148.5MHz
	{15 , 1, 0x08000000 , 1 <<  3},		// secure			148.5MHz
};

static struct div_param div_table_full[] = {
	{1, 1, 0xff     , 1<<6  , 1<<7  , 7     , 1<<5 }, // vpu_pixel_div			148.5M
	{1, 1, 0xf<<8   , 1<<11 , 0     , 7<<8  , 0    }, // clock_pixel_gate_div	148.5/2M
	{1, 1, 0xff<<12 , 1<<19 , 1<<18 , 10<<12, 0<<6 }, // svpu_pix_clk_div		108M
	{3, 1, 0x3      , 0     , 0     , 3     , 0    }, // clock_div_pixel1_doube	27M
	{3, 1, 0x5      , 0     , 0     , 5     , 1<<6 }, // clock_div_pixel1		13.5M
	{1, 1, 0xff<<23 , 1<<31 , 1<<30 , 9<<23 , 1    }, // audio_lodac_clk_div		553/(9 + 1) = 55.3M
	{2, 1, 0x1ff<<4 , 1<<12 , 1<<11 , 9<<4  , 1<<30}, // dvb_clk1_div_out		118.8M		// demod
	{2, 1, 0x1ff<<13, 1<<21 , 1<<20 , 7<<13 , 1<<29}, // dvb_clk2_div_out2		148.5M		// fec
#ifdef DVB_CHANNEL_S2
	{2, 1, 0x1ff<<22, 1<<30 , 1<<29 , 4<<22,  1<<27}, // dvb_clk3_div_out2		237M		// adc
#elif defined(DVB_CHANNEL_C)
	{2, 1, 0x1ff<<22, 1<<30 , 1<<29 , 19<<22, 1<<27}, // dvb_clk3_div_out2		59.4M		// adc
#endif
	{3, 2, 0xff<<8  , 1<<15 , 1<<14 , 3<<8  , 1<<21}, // ts_base_div				297M
	{3, 2, 0xff<<24 , 1<<31 , 1<<30 , 3<<24 , 1<<4 }, // clock_usb_12m_row		12M
};

static void USB_SetSuspendM(void)
{
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) &= ~(1<<10);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) &= ~(1<<26);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB2_CONFIG) &= ~(1<<26);
}

static void USB_ClrSuspendM(void)
{
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) |= (1<<10);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) |= (1<<26);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB2_CONFIG) |= (1<<26);
}

static void USB_SetPhyReset(void)
{
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) |= (1<<12);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) |= (1<<28);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB2_CONFIG) |= (1<<28);
}

static void USB_ClrPhyReset(void)
{
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) &= ~(1<<12);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB1_CONFIG) &= ~(1<<28);
    *(volatile unsigned int *)(USB_CONFIG_BASE + CONFIG_USB2_CONFIG) &= ~(1<<28);
}

static void usb_phy_config_ddr2(void)
{
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x18) = 0x05;

	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x18) = 0x05;

	return ;
}

static void usb_phy_config_ddr3(void)
{
#ifdef CONFIG_BGA
	// BGA
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x08) = 0x4c;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x18) = 0x05;

	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x08) = 0x4c;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x18) = 0x05;
#else
	// QFN
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x18) = 0x05;

	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x18) = 0x05;
#endif

	return ;
}

static void USB_Init(void)
{
#ifdef DDR_SEC_PAR_INCLUDE
	unsigned char ddr_info = DDR_INFO_GET();

	if ((ddr_info & 0xF) == 2)
		usb_phy_config_ddr2();
	else
		usb_phy_config_ddr3();
#else
	usb_phy_config_ddr3();
#endif

	// USB CONFIG
    *(volatile unsigned int*)(USB_CONFIG_BASE + CONFIG_USB_CONFIG)  |= (1<<25) | (1<<31) | (0x20<<8) | (0x20<<20) | (0x20<<26) | (0xf<<16);
    *(volatile unsigned int*)(USB_CONFIG_BASE + CONFIG_USB3_CONFIG) |= (1<<25) | (0x20<<20);

    USB_SetSuspendM();
    USB_SetPhyReset();
    USB_ClrPhyReset();
    USB_ClrSuspendM();
    USB_SetSuspendM();
    USB_SetPhyReset();
    USB_ClrPhyReset();

	return ;
}

static void close_macrovision(void)
{
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL0) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL1) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL2) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL3) = 0;
}

void gx_setup_pll_full_controller(void)
{
	unsigned int eth_config;
	int i = 0;
	unsigned int usb_clock_source_cfg = 0;
	unsigned int ts_clock_source_cfg = 0;

	// DTO config
	for (i = 0; i < sizeof(dto_table_full)/sizeof(struct dto_param); i++) {
		DTO_Config(&dto_table_full[i], 1);
		if (dto_table_full[i].dto == 7)
			source_config(1, 0<<28, 1);    // clock_ga_sys
		else if (dto_table_full[i].dto == 8) {
			source_config(2, 1<<23, 1);    // clock_pidfilter_sys
			source_config(2, 1<<24, 1);    // clock_gse_sys
		}
	}

	// DIV config
	for (i = 0; i < sizeof(div_table_full)/sizeof(struct div_param); i++)
		DIV_Config(&div_table_full[i], 1);

	/* usb clock source begin */
	// clock_usb_source
	usb_clock_source_cfg |= 1<<0;
	// clock_usb_48M
	usb_clock_source_cfg |= 1<<3;
	// clock_usb1_mac
	usb_clock_source_cfg |= 1<<2;
	// clock_usb_mac
	usb_clock_source_cfg |= 1<<1;

	source_config(2, usb_clock_source_cfg, 1);
	/* usb clock source end */

	// clock_adc_source
	source_config(1, 1<<31, 1);

	// clock_vpu_source
	source_config(1, 1<<5, 1);
	// clock_svpu_source
	source_config(1, 1<<6, 1);

	// dvb_adc_clk_sel
	source_config(2, 1<<8, 1);
#ifdef DVB_CHANNEL_C
	source_config(2, 1<<9, 1);
#endif

	// vdac_clk_sel
	source_config(1, 1<<8, 1);

	// clock_vpu_dac_soure
	source_config(2, 1<<6, 1);

	// clock_svpu_dac_soure
	source_config(2, 1<<7, 1);

	// clock_ADC_clk_edge
	source_config(2, 1<<10, 1);

	/* ts clock source begin */
	//ts_clk_0_source   ,clock_source_sel2[11];
	ts_clock_source_cfg |= 1<<11;
	//ts_clk_1_source   ,clock_source_sel2[12];
	ts_clock_source_cfg |= 1<<12;
	//ts_clk_2_source   ,clock_source_sel2[13];
	ts_clock_source_cfg |= 1<<13;
	//ts_clk_3_source   ,clock_source_sel2[14];
	ts_clock_source_cfg |= 1<<14;
	//ts_clk_0_edge     ,clock_source_sel2[15];
	ts_clock_source_cfg |= 1<<15;
	//ts_clk_1_edge     ,clock_source_sel2[16];
	ts_clock_source_cfg |= 1<<16;
	//ts_clk_2_edge     ,clock_source_sel2[17];
	ts_clock_source_cfg |= 1<<17;
	//ts_clk_3_edge     ,clock_source_sel2[18];
	ts_clock_source_cfg |= 1<<18;

	source_config(2, ts_clock_source_cfg, 1);
	/* ts clock source end */

	// cfg_bitosync_inv ,clock_source_sel2[20]
	//source_config(2, 1<<20, 1);

	// clock_rm_source  ,clock_source_sel2[22]
	source_config(2, 1<<22, 1);

	// eth phy interface select
	eth_config = *(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ETH_CONFIG);
	eth_config |= 0x100;
	eth_config &= ~0xc0;
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ETH_CONFIG) = eth_config; // RMII

	close_macrovision();

#ifdef DVB_CHANNEL_S2
	//adc set

	//set the  fsctrl
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0xff<<10); // fsctrl 000is 1x 11111111 = 1.996x
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (0x10<<10); // fsctrl 000is 1x 11111111 = 1.996x
	//bctrl 00110
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1f<<2);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (0x6<<2); ///70%

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1f<<24); // set i2c address
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x1d<<24); // set i2c address //1d << 2 address is 74
	//set the scl and sda is high
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x7<<29); // set scl and sdi and i2c resetz go high

	// when power down . adc is in power down  mode set opm =00;

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~((0x3<<18) | (0x1<<8) ); // adc in power down mode endacr is 0
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (1<<8); // endcr disable the duty cycle restorer

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &=  ~(0x3<<22); // use_prev_f is zero ,powerup need the use_prev_f, startcal set zero, power auto starcal

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1<<29); // i2c resetz set low keep at leas 4ns
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x1<<29); // i2c resetz set hihgh no is

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x3<<18); // i2c resetz set hihgh no is
	// set to the normal mode
	//do {
	//	adc_rdy = (*(volatile unsigned int*)(CONFIG_BASE_MMU+STATE_ADC))& 0x2 ; // [1] bit is adcrdy
	//} while (!(adc_rdy & 0x2));
	// set the port
	// DVBS set the duble I and q

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x1<<7) |(0x1<<20) ; // i2c resetz set hihgh no is
	//end
#endif
#ifdef DVB_CHANNEL_C
	//set the  fsctrl
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0xff<<10); // fsctrl 000is 1x 11111111 = 1.996x
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (0x10<<10);
	//bctrl 00110
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1f<<2);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (0x6<<2); ///70%


	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1f<<24); // set i2c address
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x1d<<24); // set i2c address //1d << 2 address is 74
	//set the scl and sda is high
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x7<<29); // set scl and sdi and i2c resetz go high

	// when power down . adc is in power down  mode set opm =00;

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~((0x3<<18) | (0x1<<8) ); // adc in power down mode endacr is 0
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (1<<8); // endcr disable the duty cycle restorer

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &=  ~(0x3<<22); // use_prev_f is zero ,powerup need the use_prev_f, startcal set zero, power auto starcal

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0x1<<29); // i2c resetz set low keep at leas 4ns
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x1<<29); // i2c resetz set hihgh no is

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |=  (0x3<<18); // i2c resetz set hihgh no is

	// set to the normal mode
	//do {
	//	adc_rdy = (*(volatile unsigned int*)(CONFIG_BASE_MMU+STATE_ADC))& 0x2 ; // [1] bit is adcrdy
	//} while (!(adc_rdy & 0x2));

	// set the port
	// DVBS set the duble I and q

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~((0x1<<7) |(0x1<<20)) ; // one port
	//end

#endif

	USB_Init();

	//modify spdif driver strength, 0xa030aa04[28:27]: 00-6.6mA, 01-13.3mA, [10-19.9mA], 11-26.5mA
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_IO_DRIVE_CTL2) &= ~(0x3 << 6);
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_IO_DRIVE_CTL2) |= 0x2 << 6;

	*(volatile unsigned int*)((0xa4e00000+(0xffad<<2))) &=  ~(0xf);
	*(volatile unsigned int*)((0xa4e00000+(0xffbf<<2))) &=  ~(0xf);
	/* 芯片有个陈旧的tlb模块,会检查各模块的ddr申请,如果申请地址在0x010000000-0x0fffffff范围内,会粗暴断联axi申请,从而导致模块卡死
	 * 配置0xa030a164 = 7可以删除该tlb模块
	 * */
	*(volatile unsigned int *)0xa030a164 = 7;
}

