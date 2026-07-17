static struct dto_param dto_table_full[] = {
	{1  , 1, 0x02a5de08 , 1 << 0 }    ,  // Audio i2s           99MHz
	{2  , 1, 0x05555555 , 1 << 1 }    ,  // Audio spdif         99MHz
	{3  , 1, 0x10000000 , 1 << 9 }    ,  // Video               297Mhz
	{4  , 1, 0x08000000 , 1 << 10}    ,  // JPEG                148.5MHZ
	{5  , 1, 0x08000000 , 1 << 11}    ,  // PP                  148.5MHZ
	{6  , 1, 0x0ccccccc , 1 << 12}    ,  // Audio decoder       237.6MHZ
	{7  , 1, 0x09245FD9 , 1 << 13}    ,  // GA                  169.714285714MHz
	{8  , 1, 0x09245FD9 , 1 << 14}    ,  // DemuxSys            169.714285714MHz
	{9  , 1, 0x05D1745D , 1 << 15}    ,  // DemuxStc            108MHz
	{15 , 1, 0x09245FD9 , 1 << 2 }    ,  // secure              169.714286714MHz
	{16 , 2, 0x10000000 , 1 << 0 }    ,  // vpu_axi             297MHz
};

static struct div_param div_table_full[] = {
	{1, 1, 0xff     , 1<<6  , 1<<7  , 7     , 1<<3 }, // vpu_pixel_div,          148.5M
	{1, 1, 0xff<<12 , 1<<19 , 1<<18 , 10<<12, 1<<4 }, // svpu_pix_clk_div        108M
	{1, 2, 0xff<<23 , 1<<31 , 1<<30 , 21<<23, 1<<16}, // audio_lodac_clk_div,    1188 / (21 + 1) = 54M
	{2, 1, 0xff<<8  , 1<<15 , 1<<14 , 11<<8 , 1<<22}, // dvb_clk1_div_out        27Mhz xtal: 90M 24Mhz xtal: 102M
#ifdef CONFIG_24M_XTAL
	{2, 1, 0xff<<16 , 1<<23 , 1<<22 , 7<<16 , 1<<21}, // dvb_clk2_div_out2       153M
#else
	{2, 1, 0xff<<16 , 1<<23 , 1<<22 , 6<<16 , 1<<21}, // dvb_clk2_div_out2       154M
#endif
	{2, 1, 0xff<<24 , 1<<31 , 1<<30 , 17<<24, 1<<19}, // dvb_clk3_div_out2       64M
	{3, 1, 0x3      , 0     , 0     , 7     , 0    }, // clock_div_pixel1        27M
	{3, 1, 0xf<<3   , 1<<6  , 1<<6  , 7<<3  , 0    }, // clock_div_pixel         148.5M
	{3, 1, 0xff<<8  , 1<<15 , 1<<14 , 2<<8  , 0    }, // ts_base_div             M
	{3, 2, 0xff<<16 , 1<<23 , 1<<22 , 43<<16, 0<<29}, // eth_phy_div             27M
	{3, 1, 0xff<<24 , 1<<31 , 1<<30 , 3<<24 , 1<<27}, // clock_usb_12m_row       12M
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

static void USB_Init(void)
{
    // USB PHY PORT CONFIG
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT1 + 0x18) = 0x05;

	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x00) = 0x1f;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x08) = 0x5c;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x14) = 0xac;
	*(volatile unsigned int*)(USB_PHY_PORT2 + 0x18) = 0x05;

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

static void eth_disable (void)
{
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_MPEG_CLK_INHIBIT2_NORM ) |= 1 << 23; // close eth clock
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_MPEG_HOT_RST_NORM) = (1 << 8); //  eth reset release
}

static void eth_enable(void)
{
	unsigned int eth_config;
	unsigned char cfg = 0;

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_MPEG_CLK_INHIBIT2_NORM ) &= ~(1 << 23); // enable eth clock
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_MPEG_HOT_RST_NORM) &= ~(1 << 8); //  eth reset
	/************** ETH clock ****************/
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(1<<23);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (1<<23); //rst
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(0x3f<<16);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (43<<16); //div to 44 27M
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(1<<22);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (1<<22); //load en
	//*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_SOURCE_SEL2) |= (1<<29); // set eth phy clock DTO div


//	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_SOURCE_SEL) &= ~(1<<28); // set eth phy from xtal to DTO div
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_SOURCE_SEL2) &= ~(1<<1); // set eth phy from xtal to DTO div
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_SOURCE_SEL2) &= ~(1<<2); // set eth phy from xtal to DTO div

	//ETH PHY set
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |= (0x1<<23); // 0 shut down, fixe en zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x3<<0); // 0 shut down, fixe en zero
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x3<<9); // 0 norm mode

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x3<<7); // 0 mode select
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |=  (0x2<<7); // 10 internal rmii and SMI

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x7<<4); // 0 27M clock
#ifdef CONFIG_24M_XTAL
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |=  (0x6<<4); // 10
#else
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |=  (0x5<<4); // 10
#endif

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x1f<<11); // 0 phy address
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |=  (0x1<<11); // 10

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0x1<<3); // 0  ephy_led_pol

	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(0xf<<16); // 0 BGS

#if 0
	gx_otp_read(0x368,1,&cfg); // to be check
	if (cfg&(1<<4))
		cfg &= 0xf;
	else
		cfg = 0x7;
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_EPHY_CONFIG) |=  (cfg<<16); // 10 BGS set
#else
	cfg = 0x7;
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_EPHY_CONFIG) |=  (cfg<<16); // 10 BGS set
#endif

	//PHY rest
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) &= ~(1<<2); // select from not xtal
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_EPHY_CONFIG) |= (1<<2); // select from not xtal

	//rmii_clk
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL) |= (1<<16); // select from not xtal
	// mii clock
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL) |= (1<<17); // select from not xtal
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_SOURCE_SEL) |= (1<<18); // select from not xtal

	// eth phy interface select
	eth_config = *(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ETH_CONFIG);
	eth_config |= 0x100;
	eth_config &= ~0xc0;
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ETH_CONFIG) = eth_config; // RMII
}

static void close_macrovision(void)
{
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL0) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL1) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL2) = 0;
	*(volatile unsigned int*)(GX_REG_VIRTUAL_BASE1 + MACROVISION_CTRL3) = 0;
}

//====== after gx_setup_pll_mini_controller() ==========
//DTO_PLL, addr=0x0030a0c0, data=0x0011102c
//DVB_PLL, addr=0x0030a0c0, data=0x004120ed
//DDR_PLL, addr=0x0030a0c0, data=0x0011102b
//DTO_1  , addr=0x0030a028, data=0xc5555555
//DTO_2  , addr=0x0030a02c, data=0xc5555555
//DTO_3  , addr=0x0030a030, data=0xd0000000
//DTO_4  , addr=0x0030a034, data=0xc8000000
//DTO_5  , addr=0x0030a038, data=0xc8000000
//DTO_6  , addr=0x0030a03c, data=0xcccccccc
//DTO_7  , addr=0x0030a040, data=0xd0000000
//DTO_8  , addr=0x0030a044, data=0xc9245fd9
//DTO_9  , addr=0x0030a048, data=0xc5d1745d
//DTO_10 , addr=0x0030a04c, data=0xc3333333
//DTO_11 , addr=0x0030a050, data=0xc1999999
//DTO_12 , addr=0x0030a054, data=0xe0000000
//DTO_13 , addr=0x0030a058, data=0xcaaaaaaa
//DTO_14 , addr=0x0030a05c, data=0xc9245fd9
//DTO_15 , addr=0x0030a060, data=0xc9245fd9
//DTO_16 , addr=0x0030a064, data=0xd0000000
//clock_div_config     , addr=0x0030a024, data=0xc58ca2c7
//clock_div2_config    , addr=0x0030a178, data=0xd1c7cb07
//clock_div3_config    , addr=0x0030a17c, data=0xc3ebc23f
//clock_source_config  , addr=0x0030a170, data=0xbfffffff
//clock_source2_config , addr=0x0030a174, data=0x3fff01df
//===========================================================

void gx_setup_pll_full_controller(void)
{
	int i = 0;
	unsigned int usb_clock_source_cfg = 0;
	unsigned int ts_clock_source_cfg = 0;
	unsigned int vdac_clock_source_cfg = 0;

	// DTO config
	for (i = 0; i < sizeof(dto_table_full)/sizeof(struct dto_param); i++) {
		if (dto_table_full[i].dto == 3) { //video
			if (DDR_FREQUENCY_CONFIG == DDR_FREQUENCY_400M)
				dto_table_full[i].div = 0x0AAAAAAA;
		}

		DTO_Config(&dto_table_full[i], 1);

		if (dto_table_full[i].dto == 8) {
			source_config(2, 1<<6 , 1);    // clock_pidfilter_sys
			source_config(2, 1<<7 , 1);    // clock_gse_sys
		}
	}

	// DIV config
	for (i = 0; i < sizeof(div_table_full)/sizeof(struct div_param); i++)
		DIV_Config(&div_table_full[i], 1);

	/* usb clock source begin */
	// clock_usb_source
	source_config(2, 1<<22, 1);
	// clock_usb_48M
	usb_clock_source_cfg |= 1<<26;
	// clock_usb1_mac
	usb_clock_source_cfg |= 1<<25;
	// clock_usb_mac
	usb_clock_source_cfg |= 1<<24;

	source_config(1, usb_clock_source_cfg, 1);
	/* usb clock source end */

	// clock_adc_source
	source_config(1, 1<<30, 1);
	source_config(1, 1<<31, 1);

	// clock_vpu_source
	source_config(1, 1<<3, 1);
	// clock_svpu_source
	source_config(1, 1<<4, 1);

	// dvb_adc_clk_sel
	source_config(1, 1<<23, 1);

	// vdac_clk_sel
	vdac_clock_source_cfg |= 1<<28;
	vdac_clock_source_cfg |= 1<<29;
	vdac_clock_source_cfg |= 1<<5 ;
	vdac_clock_source_cfg |= 1<<6 ;
	vdac_clock_source_cfg |= 1<<7 ;
	vdac_clock_source_cfg |= 1<<8 ;
	source_config(1, vdac_clock_source_cfg , 1);

	/* ts clock source begin */
	//ts_clk_0_source   ,clock_source_sel2[23];
	ts_clock_source_cfg |= 1 << 23;
	//ts_clk_1_source   ,clock_source_sel2[24];
	ts_clock_source_cfg |= 1 << 24;
	//ts_clk_2_source   ,clock_source_sel2[25];
	ts_clock_source_cfg |= 1 << 25;
	//ts_clk_0_edge     ,clock_source_sel2[26];
	ts_clock_source_cfg |= 1 << 26;
	//ts_clk_1_edge     ,clock_source_sel2[27];
	ts_clock_source_cfg |= 1 << 27;
	//ts_clk_2_edge     ,clock_source_sel2[28];
	ts_clock_source_cfg |= 1 << 28;

	source_config(2, ts_clock_source_cfg, 1);
	/* ts clock source end */

	// dvb bitosync_inv
	//source_config(2, 1<<4, 1);

	// dvb ADC_clk_edge
	source_config(2, 1<<8, 1);

	eth_enable();
	close_macrovision();

	//set the  fsctrl
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) &= ~(0xff<<10); // fsctrl 000is 1x 11111111 = 1.996x
#ifdef DVB_CHANNEL_C
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_ADC_CONFIG_0) |= (0x10<<10);
#endif
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

	USB_Init();

#ifdef ENABLE_SECURE_ALIGN
	// usb
	*(volatile unsigned int*)(USB_CONFIG_BASE + CONFIG_USB3_CONFIG) |= 1;
	// demux
	*(volatile unsigned int*)(REG_BASE_DEMUX + 0x47F0) &= ~0x3;
	*(volatile unsigned int*)(REG_BASE_DEMUX + 0x47F0) |= 0x3;
	// audio_or
	*(volatile unsigned int*)(REG_BASE_ADEC + 0x1EC) &= ~(0x3 << 1);
	*(volatile unsigned int*)(REG_BASE_ADEC + 0x1EC) |= (0x3 << 1);
	// ga
	*(volatile unsigned int*)(REG_BASE_GA + 0x34) |= 1;
#endif

	//modify spdif driver strength, 0xa030aa04[28:27]: 00-6.6mA, 01-13.3mA, [10-19.9mA], 11-26.5mA
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_IO_DRIVE_CTL2) &= ~(0x3 << 12);
	*(volatile unsigned int*)(CONFIG_BASE_MMU+CONFIG_IO_DRIVE_CTL2) |= 0x2 << 12;

	/* enable sram lowpower */
	*(volatile unsigned int*)(CONFIG_BASE_MMU+0x20C) |= 1;

	/* 芯片有个陈旧的tlb模块,会检查各模块的ddr申请,如果申请地址在0x010000000-0x0fffffff范围内,会粗暴断联axi申请,从而导致模块卡死
	 * 配置0xa030a164 = 7可以删除该tlb模块
	 * */
	*(volatile unsigned int *)0xa030a164 = 7;
}

