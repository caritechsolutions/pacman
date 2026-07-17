
**********************************************************************
本目录存放前端配置文件，该目录的配置文件与menuconfig中Frontend配置对应
配置文件的名字的命名规则:
芯片名+信道模式+自定义段(用'_'分隔)
配置内容格式:
前端号(从frontend1开始加)+配置项 = 参数
其中 //  /*  # 都认为注释
配置文件中的所有非注释的参数都会被parse_frontend_config.py脚本
解析为宏定义输出到app/module/app_frontend_board_config.h中，因此
等号左侧的配置名不能被修改，只能增加或修改frontend2，frontend3，frontend4
**********************************************************************


##################################  1  ########################################
frontend1_demod_i2c_id = 0

配置前端1的demod I2C为哪一路
一般情况下SOC芯片选择0，即第一路前端，例(GX3211,GX6605S,Taurus,Sirius,Gemini)
SOC芯片如果使用内置的I2C，则选择2，即内置I2C
一般情况下外挂芯片如果是双Tuner硬件则选择1，即第二路前端，例(GX1132,GX1133,GX1135)
一般情况下外挂芯片如果是单Tuner硬件则选择0，即第一路前端，例(ATBM78X,ATBM783X)
如果配置错误会引起寄存器读写错误，I2C报错

##################################  2  ########################################
frontend1_demod_i2c_chip_addr = 0xFF

配置前端1的demod I2C器件地址，如果SOC芯片使用的内部IPBUS该项可填任意值
目前已有的芯片一般情况下选择情况如下:
1. GX3211:一般情况下使用IPBUS，可填任意值，如果使用内部I2C则需要填0xE4
2. GX1132:0xE4
3. GX1133:0xB4
4. Taurus:一般情况下使用IPBUS，可填任意值，如果使用内部I2C则需要填0xA4
5. Sirius:一般情况下使用IPBUS，可填任意值，如果使用内部I2C则需要填0xA4
6. GX1135:0xA4
7. Gemini:一般情况下使用IPBUS，可填任意值，如果使用内部I2C则需要填0xE4
8. ATBM78X:0x80
9. ATBM783X:0x80

##################################  3  ########################################
frontend1_demod_attach = XXX_ATTACH

配置前端1的demod，目前支持的有以下几种前端:
1. GX1121D_ATTACH
2. GX1503B_DTMB_ATTACH
3. GX113X_ATTACH
4. GX3211_ATTACH
5. GX1001_ATTACH
6. GX1503_DVBC_ATTACH
7. GX1503_DTMB_ATTACH
8. GX1801_ATTACH
9. ATBM78X_ATTACH
10. ATBM888X_DVBC_ATTACH
11. ATBM888X_DTMB_ATTACH
12. ATBM783X_DVBC_ATTACH
13. ATBM783X_DVBS_ATTACH
14. GX1133_ATTACH
15. Sirius_DVBS_ATTACH
16. Sirius_DVBC_ATTACH
17. MXL683_ATTACH
18. Taurus_DVBS_ATTACH
19. Taurus_DVBC_ATTACH
20. GX1135_DVBS_ATTACH
21. Gemini_DVBT_ATTACH
22. Gemini_DVBC_ATTACH
23. Sirius_J83B_ATTACH
24. Taurus_J83B_ATTACH
以上配置会根据驱动更新而更新，具体可根据demod.h文件确认
该文件可在目录library/goxceed/ARCH-OS/include/demod.h中查看(ARCH为csky或者arm，OS为ecos或者linux)
也可以在gxfrontend/include/demod.h中查看

##################################  4  ########################################
frontend1_tuner_i2c_id = 0

配置前端1的tuner I2C为哪一路，一般情况下与frontend1_demod_i2c_id配置相同
如果Tuner的I2C挂载与demod不是同一路上，则需要根据gxloader中的board-init.c中选择的SDAx SCLx配置
如果Tuner的I2C连接为Master模式，则该项需要置0xFF
如果配置错误可能会引起I2C报错

##################################  5  ########################################
frontend1_tuner_i2c_chip_addr = 0xFF

配置前端1的tuner的I2C器件地址，目前支持的Tuner配置如下:
1. AV2018:0xC0
2. AV2020:0xC0
3. MXL608:0xC0
4. TDA18250:0xC0
5. TDA18250A:0xC0
6. TDA18273
7. R836:0xF8
8. R848
9. R910
10. SI2141
11. RDA5815M:0x18
12. ATBM2040
13. ATBM2030
14. SHARP7306:0xC0

##################################  6  ########################################
frontend1_iq = 0

配置前端1的IQ，参数为0(取反)或1(不取反)，与硬件连接相关
取反:0 不取反:1, 硬件设计上有I+ I- Q+ Q- 4条连线，有以下4种连接方式
 1.I+ I- Q+ Q- 2. I+ I- Q+ Q- 3. I+ I- Q+ Q- 4. I+ I- Q+ Q-
   |  |  |  |     |  |  |  |     |  |  |  |     |  |  |  |
   I+ I- Q+ Q-    I- I+ Q+ Q-    I+ I- Q- Q+    I- I+ Q- Q+
原则上只有1路硬件连接取反，IQ就要配0，2路都连接取反或者都没有取反则配1，
即连接方式1和4配置1，连接方式2和3配置0

##################################  7  ########################################
frontend1_lnb_power_gpio = V_GPIO_LNB_A

配置前端1的使能LNB管脚，如果硬件是通过GPIO来控制LNB ON/OFF的，则需要配置该项，可直接配置虚拟GPIO号，
虚拟GPIO号在gxloader中的board-init.c中定义，该值取值范围为0≤x≤255，否则该配置无效，
除填写虚拟GPIO号以外，也可以按上述示例填写宏定义

##################################  8  ########################################
frontend1_pol_ctrl_enable = 0

配置是否由前端驱动控制polar H/V切换, 1:由驱动控制(不配置默认由驱动控制), 0:不由驱动控制

##################################  9  ########################################
frontend1_lnb_ctrl_enable = 0

配置是否由前端驱动控制lnb off/13/18/切换, 1:由驱动控制(不配置默认由驱动控制), 0:不由驱动控制

##################################  10  ########################################
frontend1_lnb_invert = 1

配置前端1的LNB ON/OFF，是否需要反转，即0:不反转，GPIO拉高LNB ON；1:反转，GPIO拉高LNB OFF

##################################  11  ########################################
frontend1_polar_invert = 1

配置前端1的polar是否需要反转，即0:不反转，13/18V输出也是13/18V；1:反转，13/18V输出为18/13V

##################################  12  ########################################
frontend1_ts_pin_config_00 = ERRS
frontend1_ts_pin_config_01 = ERRS
frontend1_ts_pin_config_02 = ERRS
frontend1_ts_pin_config_03 = ERRS
frontend1_ts_pin_config_04 = ERRS
frontend1_ts_pin_config_05 = ERRS
frontend1_ts_pin_config_06 = ERRS
frontend1_ts_pin_config_07 = ERRS
frontend1_ts_pin_config_08 = ERRS
frontend1_ts_pin_config_09 = ERRS
frontend1_ts_pin_config_10 = ERRS
frontend1_ts_pin_config_11 = ERRS

配置前端1的TS各个管脚输出功能，管脚和管脚定义如下:
硬件物理管脚  管脚功能
TS00          DATA0
TS01          DATA1
TS02          DATA2
TS03          DATA3
TS04          DATA4
TS05          DATA5
TS06          DATA6
TS07          DATA7
TS08          CLK
TS09          VALID
TS10          SYNC
TS11          ERROR
              DATAS
              VALIDS
              SYNCS
              ERRS
每个管脚按照硬件的连线配置对应的功能，例如以下为配置某款芯片的串行3线TS输出
frontend1_ts_pin_config_00 = CLK
frontend1_ts_pin_config_01 = SYNC
frontend1_ts_pin_config_02 = DATA0
frontend1_ts_pin_config_03 = ERRS
frontend1_ts_pin_config_04 = ERRS
frontend1_ts_pin_config_05 = ERRS
frontend1_ts_pin_config_06 = ERRS
frontend1_ts_pin_config_07 = ERRS
frontend1_ts_pin_config_08 = ERRS
frontend1_ts_pin_config_09 = ERRS
frontend1_ts_pin_config_10 = ERRS
frontend1_ts_pin_config_11 = ERRS
管脚功能定义在library/goxceed/ARCH-OS/include/frontend/frontend.h中查看(ARCH为csky或者arm，OS为ecos或者linux)
枚举ts_pin_t的定义

##################################  13  ########################################
frontend1_ts_mode = SERIAL

配置前端1的TS输出模式，配置为PARALLEL或者SERIAL，需要根据硬件配置
参数可参考在library/goxceed/ARCH-OS/include/frontend/frontend.h中(ARCH为csky或者arm，OS为ecos或者linux)
枚举ts_mode_t的定义

##################################  14  ########################################
frontend1_if_freq = 5000

配置前端1的tuner中频，单位KHz，目前配置用到的有4500、4570、5000，并且只针对DVBC和DVBT/T2，
使用R836，R848，R850, R910时可配置成4570或者5000，别的tuner可配置成4500或者5000，
目前只支持Sirius_DVBC, Taurus_DVBC, Gemini_DVBC, Gemini_DVBT/T2，Cygnus_DVBC，Cygnus_DVBT/T2

##################################  15  ########################################
frontend1_tuner_crystal = 24

配置前端1的tuner晶振，单位MHz，目前配置用到的有16，24，27MHz，并且只针对DVBC/C2和DVBT/T2，
目前只支持tuner: mxl608, R836, R850

##################################  16  ########################################
frontend1_tuner_xtal_cap = 24

配置前端1的tuner的晶振匹配电容，单位为pf，目前使用mxl608时，如果晶振为24M，在sirius/taurus
DVBC时，需要配置31pf，否则配置0pf，如果晶振为16M时，则都配置为31pf

##################################  17  ########################################
frontend1_tuner_clock_out = 1

配置前端1的tuner的时钟输出使能，1使能输出，0不输出

##################################  18  ########################################
frontend1_tuner_loop_out = 1

配置前端1的tuner的环通输出使能，1使能输出，0不输出

##################################  19  ########################################
frontend1_tuner_connection = FE_DIFFERENTIAL

配置前端1的tuner连接方式，
FE_DIFFERENTIAL 为差分
FE_SINGLE_END 为单端

##################################  20  ########################################
frontend1_demux_ts_src = DEMUX_TS1

配置DEMUX的TS_SOURCE，可以配置为枚举，也可以直接写数值
如果没有配置该项，则按默认计算方式，即
如果是非COMBO方案，则TS_SOURCE从0开始累加
如果是COMBO方案，则对于COMBO的信道TS_SOURCE都按同一个值处理

##################################  21  ########################################
frontend1_tuner1_adapt_attach = MXL608_T_ATTACH
frontend1_tuner1_adapt_i2c_chip_addr = 0xc0,0xc4
frontend1_tuner1_adapt_i2c_id = 0
frontend1_tuner1_adapt_iq = 0
frontend1_tuner1_adapt_crystal = 24
frontend1_tuner1_adapt_if_freq = 4500
frontend1_tuner1_adapt_xtal_cap = 0
frontend1_tuner1_adapt_clock_out = 0
frontend1_tuner1_adapt_loop_out = 1

配置Tuner的自适应功能，如果需要自适应多个tuner，按上述示例则需要添加frontend1_tuner2_adapt_xxx
以此类推，这其中的自适应参数，如果不需要则可以删除配置，但attach和i2c_chip_addr为必需配置不可删除，
i2c_chip_addr目前可最多支持4个，如需修改则需要修改宏MAX_DEV_ADDR_NUM

##################################  22  ########################################
frontend1_ts4pin_switch = 2,0;3,1
frontend1_pin1_switch = 10,37,69,0 //TSCLK
frontend1_pin2_switch = 22,78,0xff,0 //TSSYNC

配置管脚复用及TS是否需要输出，一般用于COMBO方案中，在TS管脚不够用时需要进行动态复用，
ts4pin_switch:此处填写的是当切换到此解调时，需要开关哪些TS，上述例子中指的是切换
  到frontend1的解调时，2和0代表frontend2的TS需要关闭，3和1代表frontend3的TS需要打开，
  两组配置需要用分号隔开，可支持多个frontend同时配置
  注意:每组的第一个参数为frontend ID从1开始，第二个参数为TS开关，限填0或1
pin1_switch: 中的1是计数，最多脚本中只支持12个，后面的参数与gxloader中
  board-init.c中的配置相对应，以下面chip_core_num=32的pin脚为例展示接口使用
---------------------------------------------------------------------------------
chip_core_num | bit0 | bit1    | bit2   | func_select   // package_num | func_list
{32,              10,    37,       69,              0}, // NC          | DiSEqCi_PORT10_TSI1DATA5_ADCOCLK

frontend1_pin1_switch = 10,37,69,0
---------------------------------------------------------------------------------

##################################  23  ########################################
frontend1_agc_gpio = 19
frontend1_hv_sel_gpio = 2
frontend1_diseqc_out_gpio = 3
frontend1_diseqc_in_gpio = 4

配置前端1的AGC，13/18V切换，DISEQC_IN和DISEQC_OUT的管脚复用，配置的是真实GPIO号，
仅适用于单解调的管脚复用配置


##################################  24  ########################################
frontend1_demod1_adapt_i2c_id = 0
frontend1_demod1_adapt_i2c_chip_addr = 0xE4,0xA4
frontend1_demod1_adapt_attach = Cygnus_DVBT_ATTACH
frontend1_demod1_adapt_lnb_power_gpio = V_GPIO_LNB_A
frontend1_demod1_adapt_lnb_ctrl_enable = 0
frontend1_demod1_adapt_lnb_invert = 1
frontend1_demod1_adapt_pol_ctrl_enable = 0
frontend1_demod1_adapt_ts4pin_switch = 2,0;2,1
frontend1_demod1_adapt_pin1_switch = 10,37,69,1 //port10,LNB POWER
frontend1_demod1_adapt_pin2_switch = 11,38,0xff,0 //HVSEL
frontend1_demod1_adapt_pin3_switch = 13,40,0xff,0 //AGC
frontend1_demod1_adapt_ts_pin_config_00 = CLK
frontend1_demod1_adapt_ts_pin_config_01 = SYNC
frontend1_demod1_adapt_ts_pin_config_02 = DATA0
frontend1_demod1_adapt_ts_pin_config_03 = ERRS
frontend1_demod1_adapt_ts_pin_config_04 = ERRS
frontend1_demod1_adapt_ts_pin_config_05 = ERRS
frontend1_demod1_adapt_ts_pin_config_06 = ERRS
frontend1_demod1_adapt_ts_pin_config_07 = ERRS
frontend1_demod1_adapt_ts_pin_config_08 = ERRS
frontend1_demod1_adapt_ts_pin_config_09 = ERRS
frontend1_demod1_adapt_ts_pin_config_10 = ERRS
frontend1_demod1_adapt_ts_pin_config_11 = ERRS
frontend1_demod1_adapt_ts_mode = SERIAL
frontend1_demod1_adapt_agc_gpio = 19
frontend1_demod1_adapt_hv_sel_gpio = 2
frontend1_demod1_adapt_diseqc_out_gpio = 3
frontend1_demod1_adapt_diseqc_in_gpio = 4
frontend1_demod1_adapt_demux_ts_src = DEMUX_TS1

配置demod及其前端的配置参数的自适应功能，如果需要自适应多个demod，按上述示例则需要添加frontend1_demod2_adapt_xxx
以此类推，这其中的自适应参数，如果不需要则可以删除配置，但attach和i2c_chip_addr以及i2c_id为必需配置不可删除，i2c_chip_addr目前可最多支持4个，如需修改则需要修改宏MAX_DEV_ADDR_NUM

################################################################################
根据以上配置以此类推
配置前端2...
frontend2...
配置前端3...
frontend3...
配置前端4...
frontend4...


