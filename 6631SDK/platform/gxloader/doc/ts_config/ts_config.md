# ts_config

Demux模块支持4路输入源，其中前3路输入源会有一路固定来自内置并行Demod，其余几路可以来自外置并行Demod、外置串行Demod，
另外部分芯片还支持来自内置T2MI。最后一路输入源固定来自CPU，即CPU可以通过接口把内存中的数据输入给Demux

## TS_MODE_CONFIG

note1:  tsx = ts0 or ts1 or ts2 or ts3
note2:  可能需要配置管脚复用，具体配置咨询硬件
note3:  该接口一般在board-init.c文件中的board_init接口中调用

```c
int ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select);
```

- [ts_port] : TS端口选择，具体配置咨询硬件
	```
	0: set ts0
	1: set ts1
	2: set ts2
	3: set ts3
	```
- [port_mode] : 使用的是串行TS还是并行TS，具体配置咨询硬件
	```
	0: parallel mode
	1: serial mode
	```

- [pin_config] :
	- gx3211/gx6605s/gx3113c/sirius
		- 当port_mode为0(parallel mode)时，该值可不用关心
		- 当port_mode为1(serial mode)时，对于TSIx中的x代表ts_port，具体配置咨询硬件
			- gx3211/gx6605s/gx3113c/sirius,x可以是1/2/3中的任意值
			- gx3201芯片, x可以是1/2中的任意值
			```c
			mode   function list                 pin list
			0:     tsx_serial_in_clk       =     TSIxCLK ;
				   tsx_serial_in_sync      =     TSIxSYNC  ;
				   tsx_serial_in_valid     =     TSIxVALID ;
				   tsx_serial_in_dat       =     TSIxDATA0 ;
			1:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxSYNC  ;
				   tsx_serial_in_valid     =     TSIxVALID ;
				   tsx_serial_in_dat       =     TSIxDATA7 ;
			2:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA7 ;
				   tsx_serial_in_valid     =     TSIxDATA6 ;
				   tsx_serial_in_dat       =     TSIxDATA5 ;
			3:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA7 ;
				   tsx_serial_in_valid     =     TSIxDATA5 ;
				   tsx_serial_in_dat       =     TSIxDATA6 ;
			4:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA6 ;
				   tsx_serial_in_valid     =     TSIxDATA7 ;
				   tsx_serial_in_dat       =     TSIxDATA5 ;
			5:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA6 ;
				   tsx_serial_in_valid     =     TSIxDATA5 ;
				   tsx_serial_in_dat       =     TSIxDATA7 ;
			6:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA5 ;
				   tsx_serial_in_valid     =     TSIxDATA7 ;
				   tsx_serial_in_dat       =     TSIxDATA6 ;
			7:
				   tsx_serial_in_clk       =     TSIxCLK   ;
				   tsx_serial_in_sync      =     TSIxDATA5 ;
				   tsx_serial_in_valid     =     TSIxDATA6 ;
				   tsx_serial_in_dat       =     TSIxDATA7 ;
			```
	- taurus/gemini
		- 当port_mode为0(parallel mode)时，该值可不用关心
		- 当port_mode为1(serial mode)时，对于TSIx中的x代表ts_port，目前x只能设置1，具体配置咨询硬件
		- pin_config 的前18比特位对应各个pin脚选择
			```
			BIT2~0: valid pin select
			BIT5~3: sync pin select
			BIT8~6: dat[0] pin select
			BIT10~7: dat[1] pin select
			BIT13~11: dat[2] pin select
			BIT17~14: dat[3] pin select
			```
		- 每个BIT范围区域的值对应选择的pin脚:
			```
			0x0: TSIxVALID
			0x1: TSIxSYNC
			0x2: TSIxDATA0
			0x3: TSIxDATA1
			0x4: TSIxDATA2
			0x5: TSIxDATA3
			```
- [valid_enable] : 具体配置咨询硬件
	```
	0: TSIxVALID invalid
	1: TSIxVALID valid
	```

- [sync_enable] :
	- 当port_mode为0(parallel mode)时，具体配置咨询硬件
	- 当port_mode为1(serial mode)时，固定置1
		```
		0: TSIxSYNC invalid
		1: TSIxSYNC valid
		```

- [endian_select] : 与edge_select一起，由软件进行组合测试，共2*2种
	```
	0: little endian
	1: big endian
	```
	- ex. if data=0x47
		```
		little endian & parallel mode: TSIxDATA7=0,D6=1,D5=0,D4=0,D3=0,D2=1,D1=1,D0=1
		big    endian & parallel mode: TSIxDATA7=1,D6=1,D5=1,D4=0,D3=0,D2=0,D1=1,D0=0
		little endian & serial   mode: first bit=1,2nd=1,3rd=1,4th=0,5th=0,6th=0,7th=1,8th=0
		big    endian & serial   mode: first bit=0,2nd=1,3rd=0,4th=0,5th=0,6th=1,7th=1,8th=1
		```

- [edge_select] : 见endian说明
	```
	0:failing edge
	1:rising edge
	-1:not config
	default->rising edge
	```

## 管脚复用配置
- sirius

根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1/TSI2/TSI3), 根据func_list修改func_sel

```c
 /*      size | chip_core_num | 0_bit | 1_bit | 2_bit | func_sel */ /* package_num | func_list */
        {_RIGHT_,    11,     41,    122,         MP_INV_V,  0},           //NC     |PORT09/TSI1VALID/TSOVALID
        {_RIGHT_,    13,     42,    101,         MP_INV_V,  0},           //NC     |PORT10(PMUPORT09)/SC1CLK/TSI1DATA0/TSODATA0
        {_RIGHT_,    14,     43,    102,         MP_INV_V,  0},           //NC     |PORT11(PMUPORT10)/SC1RST/TSI1DATA1/TSODATA1
        {_RIGHT_,    15,     44,    103,         MP_INV_V,  0},           //NC     |PORT12(PMUPORT11)/SC1PWR/TSI1DATA2/TSODATA2
        {_RIGHT_,    16,     45,    104,         MP_INV_V,  0},           //NC     |PORT13(PMUPORT12)/SC1CD/TSI1DATA3/TSODATA3
        {_RIGHT_,    17,     46,    105,         MP_INV_V,  0},           //NC     |PORT14(PMUPORT13)/SC1DATA/TSI1DATA4/TSODATA4
        {_RIGHT_,    19,     47,    106,         133,       0},           //NC     |PORT15(PMUPORT14)/UART2TX/TSI1DATA5/TSODATA5/NFDATA5
        {_RIGHT_,    20,     48,    107,         134,       2},           //NC     |PORT16(PMUPORT15)/UART2RX/TSI1DATA6/TSODATA6/NFDAT4
        {_RIGHT_,    21,     49,    108,         135,       2},           //NC     |PORT17(PMUPORT16)/SDA2/TSI1DATA7/TSODATA7/NFDAT3
        {_RIGHT_,    22,     50,    109,         136,       2},           //NC     |PORT18(PMUPORT17)/SCL2/TSI1CLK/TSOCLK/NFDAT2
        {_RIGHT_,    23,     51,    110,         MP_INV_V,  0},           //NC     |PORT19/TSI1SYNC/TSOSYNC
        {_RIGHT_,    38,     52,    120,         MP_INV_V,  0},           //NC     |PORT20/TSI2VALID/TSOVALID
        {_RIGHT_,    39,     53,    120,         MP_INV_V,  0},           //NC     |PORT21/TSI2DATA0/TSODATA0
        {_RIGHT_,    40,     54,    120,         MP_INV_V,  0},           //NC     |PORT22/TSI2DATA1/TSODATA1
        {_RIGHT_,    41,     55,    120,         MP_INV_V,  0},           //NC     |PORT23/TSI2DATA2/TSODATA2
        {_RIGHT_,    42,     56,    120,         MP_INV_V,  0},           //NC     |PORT24/TSI2DATA3/TSODATA3
        {_RIGHT_,    44,     57,    120,         MP_INV_V,  0},           //NC     |PORT25/TSI2DATA4/TSODATA4
        {_RIGHT_,    45,     58,    120,         MP_INV_V,  0},           //NC     |PORT26/TSI2DATA5/TSODATA5
        {_RIGHT_,    46,     59,    120,         MP_INV_V,  0},           //NC     |PORT27/TSI2DATA6/TSODATA6
        {_RIGHT_,    47,     60,    120,         MP_INV_V,  0},           //NC     |PORT28/TSI2DATA7/TSODATA7
        {_RIGHT_,    48,     61,    120,         MP_INV_V,  0},           //NC     |PORT29/TSI2SCLK/TSOCLK
        {_RIGHT_,    50,     62,    120,         MP_INV_V,  0},           //NC     |PORT30/TSI2SYNC/TSOSYNC
        {_LEFT_,     58,     81,    MP_INV_V,    MP_INV_V,  0},           //NC     |PORT83(GBPORT19)/TSI3VALID
        {_LEFT_,     59,     82,    121,         MP_INV_V,  0},           //NC     |PORT84(GBPORT20)/TSI3DATA0/TSISVALID
        																  //NC     |PORT85(GBPORT21)/TSI3DATA1/TSISDATA
        																  //NC     |PORT86(GBPORT22)/TSI3DATA2/TSISCLK
        																  //NC     |PORT87(GBPORT23)/TSI3DATA3/TSISSYNC
        {_LEFT_,     64,     83,    MP_INV_V,    MP_INV_V,  0},           //NC     |PORT88(GBPORT24)/TSI3DATA4
        	                                                              //NC     |PORT89(GBPORT25)/TSI3DATA5
        {_LEFT_,     66,     84,    MP_INV_V,    MP_INV_V,  0},           //NC     |PORT90(GBPORT26)/TSI3DATA6
                                                                       	  //NC     |PORT91(GBPORT27)/TSI3DATA7
                                                                       	  //NC     |PORT92(GBPORT28)/TSI3CLK
                                                                       	  //NC     |PORT93(GBPORT29)/TSI3SYNC
```
- taurus

根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1/TSI2), 根据func_list修改func_sel

```c
 /*      size | chip_core_num | 0_bit | 1_bit | 2_bit | func_sel */ /* package_num | func_list */
        {_RIGHT_,    5,        14,        70,          128,       0},  //NC          |SC1CLK/PORT21/TSIDATA7/SDBGTDI/DEVADDR0
        {_RIGHT_,    6,        15,        71,          129,       0},  //NC          |SC1RST/PORT22/TSIDATA6/SDBGTDO
        {_RIGHT_,    7,        16,        72,          130,       0},  //NC          |SC1PWR/PORT23/TSIDATA5/SDBGTMS
        {_RIGHT_,    8,        17,        73,          131,       0},  //NC          |SC1CD/PORT24/TSIDATA4/SDBGTCK
        {_RIGHT_,    9,        18,        74,          132,       0},  //NC          |SC1DATA/PORT25/TSIDATA3/SDBGTRST
        {_RIGHT_,    13,       19,        75,          MP_INV_V,  0},  //NC          |NULL/PORT26/TSIDATA2/DEVADDR1
        {_RIGHT_,    14,       20,        76,          MP_INV_V,  0},  //NC          |HVSEL/PORT27/TSIDATA1
        {_RIGHT_,    15,       21,        77,          MP_INV_V,  0},  //NC          |DiSEqCo/PORT28/TSIDATA0
        {_RIGHT_,    16,       22,        78,          MP_INV_V,  0},  //NC          |AGC/PORT29/TSICLK
        {_RIGHT_,    17,       23,        MP_INV_V,    MP_INV_V,  0},  //NC          |PORT30/TSISYNC
        {_RIGHT_,    18,       24,        MP_INV_V,    MP_INV_V,  0},  //NC          |PORT31/TSIVALID
        {_RIGHT_,    25,       25,        81,          MP_INV_V,  0},  //NC          |TSODATA7/PORT32/TSI2DATA7
        {_RIGHT_,    26,       26,        82,          MP_INV_V,  0},  //NC          |TSODATA6/PORT33/TSI2DATA6
        {_RIGHT_,    27,       27,        83,          MP_INV_V,  0},  //NC          |TSODATA5/PORT34/TSI2DATA5
        {_RIGHT_,    28,       28,        84,          MP_INV_V,  0},  //NC          |TSODATA4/PORT35/TSI2DATA4
        {_RIGHT_,    29,       29,        85,          MP_INV_V,  0},  //NC          |TSODATA3/PORT36/TSI2DATA3
        {_RIGHT_,    36,       30,        86,          MP_INV_V,  0},  //NC          |TSODATA2/PORT37/TSI2DATA2
        {_RIGHT_,    37,       31,        87,          MP_INV_V,  0},  //NC          |TSODATA1/PORT38/TSI2DATA1
        {_RIGHT_,    38,       32,        88,          MP_INV_V,  0},  //NC          |TSODATA0/PORT39/TSI2DATA0
        {_RIGHT_,    39,       33,        89,          MP_INV_V,  0},  //NC          |TSOCLK/PORT40/TSI2CLK
        {_RIGHT_,    40,       34,        90,          MP_INV_V,  0},  //NC          |TSOSYNC/PORT41/TSI2SYNC
        {_RIGHT_,    41,       35,        91,          MP_INV_V,  0},  //NC          |TSOVALID/PORT42/TSI2VALID
```
- gemini

根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1/TSI2), 根据func_list修改func_sel

```c
/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
 /*      size | chip_core_num | 0_bit | 1_bit | 2_bit | func_sel */ /* package_num | func_list */
        {_BOTTOM_,   47,     20,          74,          128,       0},           //NC          |TSIDATA7/PORT06(PMUPORT06)/TSODATA7/SC1CLK/ADCBIT9/SDBGTDI
        {_BOTTOM_,   48,     21,          75,          129,       0},           //NC          |TSIDATA6/PORT07(PMUPORT07)/TSODATA6/SC1RST/ADCBIT8/SDBGTDO/NC/I2SDATA
        {_BOTTOM_,   49,     22,          76,          130,       0},           //NC          |TSIDATA5/PORT08(PMUPORT08)/TSODATA5/SC1PWR/ADCBIT7/SDBGTMS/NC/I2SBCK
        {_BOTTOM_,   50,     23,          77,          131,       0},           //NC          |TSIDATA4/PORT09(PMUPORT09)/TSODATA4/SC1CD/ADCBIT6/SDBGTCK/NC/I2SLRCK
        {_RIGHT_,    1,      24,          78,          132,       0},           //NC          |TSIDATA3/PORT10(PMUPORT10)/TSODATA3/SC1DATA/ADCBIT5/SDBGTRST/NC/I2SMCK
        {_RIGHT_,    13,     25,          79,          135,       0},           //NC          |TSIDATA2/PORT11(PMUPORT11)/TSODATA2/I2SDATA/ADCBIT4
        {_RIGHT_,    14,     26,          80,          136,       0},           //NC          |TSIDATA1/PORT12(PMUPORT12)/TSODATA1/I2SBCK/ADCBIT3
        {_RIGHT_,    15,     27,          81,          137,       0},           //NC          |TSIDATA0/PORT13(PMUPORT13)/TSODATA0/I2SLRCK/ADCBIT2
        {_RIGHT_,    16,     28,          82,          138,       0},           //NC          |TSICLK/PORT14(PMUPORT14)/TSOCLK/I2SMCK/ADCBIT1
        {_RIGHT_,    17,     29,          83,          133,       0},           //NC          |TSISYNC/PORT15/TSOSYNC/UART2TX/ADCBIT0/SDA2
        {_RIGHT_,    18,     30,          84,          134,       0},           //NC          |TSIVALID/PORT16/TSOVALID/UART2RX/ADCCLK/SCL2
        {_RIGHT_,    30,     31,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA3/PORT17
        {_RIGHT_,    31,     32,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA2/PORT18
        {_RIGHT_,    32,     33,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA1/PORT19
        {_RIGHT_,    33,     34,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA0/PORT20
        {_RIGHT_,    34,     35,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISCLK/PORT21
        {_RIGHT_,    35,     36,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISSYNC/PORT22
        {_RIGHT_,    36,     37,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISVALID/PORT23
```
- gx3211

根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1/TSI2/TSI3), 根据func_list修改func_sel

```c
	/* NRE PIN MULTIPLEX TABLE*/
	struct mulpin_config_s mulpin_table[] = {
/* chip_package: generic */
/* chip_core_num | 0_bit | 1_bit |  2_bit  |func_sel*/ /* package_num | func_list */
		{126,       83,     115,    MP_INV_V,  0},  // NC  | TSI3DAT5_PORT51_NFDAT3
		{128,       84,     116,    MP_INV_V,  0},  // NC  | TSI3DAT6_PORT52_NFDAT2
		{130,       85,     117,    MP_INV_V,  0},  // NC  | TSI3DAT7_PORT53_NFDAT1
		{132,       86,     118,    MP_INV_V,  0},  // NC  | TSI3CLK_PORT54_NFDAT0
		{136,       5,      37,     MP_INV_V,  0},  // NC  | TSI1DATA0_PORT5_NFWE_BTDATA0
		{137,       6,      38,          136,  0},  // NC  | TSI1DATA1_PORT6_SC1CLK_BTDATA1_SCTDI
		{138,       7,      39,          136,  0},  // NC  | TSI1DATA2_PORT7_SC1RST_BTDATA2_SCTDO
		{139,       8,      40,          136,  0},  // NC  | TSI1DATA3_PORT8_SC1PWR_BTDATA3_SCTMS
		{140,       9,      41,          136,  0},  // NC  | TSI1DATA4_PORT9_SC1CD_BTDATA4_SCTCK
		{141,       10,     42,          136,  0},  // NC  | TSI1DATA5_PORT10_SC1DATA_BTDATA5_SCTRST
		{142,       11,     43,     MP_INV_V,  0},  // NC  | TSI1DATA6_PORT11_NFDAT7_BTDATA6
		{143,       12,     44,     MP_INV_V,  0},  // NC  | TSI1DATA7_PORT12_NFDAT6_BTDATA7
		{144,       13,     45,     MP_INV_V,  0},  // NC  | TSI1CLK_PORT13_NFDAT5_BTCLK
		{149,       87,     119,    MP_INV_V,  0},  // NC  | SCANPWDCLk_TSI2DATA0_PORT55
		{150,       88,     120,    MP_INV_V,  0},  // NC  | SCANPWDDAT_TSI2DATA1_PORT56
		{151,       89,     121,    MP_INV_V,  0},  // NC  | SCANPWDSET_TSI2DATA2_PORT57
		{152,       90,     122,    MP_INV_V,  0},  // NC  | UARTTXICAM1_TSI2DATA3_PORT58
		{153,       91,     123,    MP_INV_V,  0},  // NC  | UARTRXICAM1_TSI2DATA4_PORT59
		{154,       92,     124,    MP_INV_V,  0},  // NC  | UARTTXICAM2_TSI2DATA5_PORT60
		{155,       93,     125,    MP_INV_V,  0},  // NC  | UARTRXICAM2_TSI2DATA6_PORT61
		{156,       94,     126,    MP_INV_V,  0},  // NC  | UARTTXICAM3_TSI2DATA7_PORT62
		{158,       95,     127,    MP_INV_V,  0},  // NC  | UARTRXICAM3_TSI2SCLK_PORT63
	};
```
- gx6605s
根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1), 根据func_list修改func_sel

```c
/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
 /* chip_core_num | bit0 | bit1    | bit2       | func_select */   /* package_num | func_list */
        {26,        5,     32,       64,          1},              // NC          | SC1CLK_PORT05_TSI1DATA0_AJTDI_DVBFSYNC_ADCODATA5
        {27,        6,     33,       65,          0},              // NC          | SC1RST_PORT06_TSI1DATA1_AJTDO_ADCODATA6
        {28,        7,     34,       66,          1},              // NC          | SC1PWR_PORT07_TSI1DATA2_AJTMS_ADCODATA7
        {29,        8,     35,       67,          1},              // NC          | SC1CD_PORT08_TSI1DATA3_AJTCK_ADCODATA8
        {31,        9,     36,       68,          0},              // NC          | SC1DATA_PORT09_TSI1DATA4_AJRST_ADCODATA9
        {32,        10,    37,       69,          1},              // NC          | DiSEqCi_PORT10_TSI1DATA5_ADCOCLK
        {33,        11,    38,       MP_INV_V,    0},              // NC          | HVSEL_PORT11_TSI1DATA6
        {34,        12,    39,       MP_INV_V,    0},              // NC          | DiSEqCo_PORT12_TSI1DATA7
        {35,        13,    40,       MP_INV_V,    0},              // NC          | AGC_PORT13_TSI1CLK
};
```
- gx3113c
根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1), 根据func_list修改func_sel

```c
/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
 /* chip_core_num | l_bit | h_bit | func_select */    /* package_num | func_list */
    {141, 48, MULPIN_INVALID_VALUE, 0},           // NC          | TSIVALID_PORT48
    {142, 49, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT0_PORT49
    {143, 50, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT1_PORT50
    {144, 51, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT2_PORT51
    {145, 52, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT3_PORT52
    {146, 53, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT4_PORT53
    {147, 54, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT5_PORT54
    {148, 55, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT6_PORT55
    {149, 56, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT7_PORT56
    {150, 57, MULPIN_INVALID_VALUE, 0},           // NC          | TSICLK_PORT57
    {151, 58, MULPIN_INVALID_VALUE, 0},           // NC          | TSISYNC_PORT58

};
```
- gx3201
根据具体的TS_MODE_CONFIG来配置管脚复用具体选择的TS端口(TSI1/TSI2), 根据func_list修改func_sel

```c
/* NRE PING MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
        { _RIGHT_,  39, 28, 76, 0},                             //TSI2DATA5_PORT28_TSODATA5
        { _RIGHT_,  40, 29, 76, 0},                             //TSI2DATA6_PORT29_TSODATA6
        { _RIGHT_,  41, 30, 76, 0},                             //TSI2DATA7_PORT30_TSODATA7
        { _RIGHT_,  42, 31, 76, 0},                             //TSI2CLK_PORT31_TSOCLK
        { _RIGHT_,  44, 32, 76, 0},                             //TSI2SYNC_PORT32_TSOSYNC
        { _RIGHT_,  45, 33, MULPIN_INVALID_VALUE, 0},           //TSI1VALID_PORT33
        { _RIGHT_,  46, 34, 73, 0},                             //TSI1DATA0_PORT34_TSI2DATA5
        { _RIGHT_,  47, 35, 73, 0},                             //TSI1DATA1_PORT35_TSI2DATA6
        { _RIGHT_,  48, 36, 73, 0},                             //TSI1DATA2_PORT36_TSI2DATA7
        { _RIGHT_,  49, 37, 73, 0},                             //TSI1DATA3_PORT37_TSI2CLK
        { _RIGHT_,  62, 38, 68, 0},                             //TSI1DATA4_PORT38_SC1CLK_NFDAT7
        { _RIGHT_,  63, 39, 68, 0},                             //TSI1DATA5_PORT39_SC1RST_NFDAT6
        { _RIGHT_,  64, 40, 68, 0},                             //TSI1DATA6_PORT40_SC1PWR_NFDAT5
        { _RIGHT_,  65, 41, 68, 0},                             //TSI1DATA7_PORT41_SC1CD_NFDAT4
        { _RIGHT_,  66, 42, 68, 0},                             //TSI1CLK_PORT42_SC1DATA_NFDATA3
        { _RIGHT_,  67, 43, MULPIN_INVALID_VALUE, 0},           //TSI1SYNC_PORT43
};
```
