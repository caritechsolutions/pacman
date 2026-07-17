# 镜像编译

build 编译系统，执行 ./build 可以查看编译命令。

```
./build <chip> <board> release

  chip    list : gx3201|gx3113c|gx3211|gx6605s|sirius|taurus

  board   list : gx3201  : 3201-dtmb|3201-dvbc|3201-dvbt2|3201e-dvbc|3231|3235-dvbt2|6601-dvbs2|6601e-dvbs2|6602-dvbs2|6605-dvbs2|generic
                 gx3113c : 3001h|3001kv|3011c|3012h|3113c|3113d|3115|6131|generic
                 gx3211  : 3113h|3113r|3113s|3201h|3202-dvbc|3211|3212-dvbc|6132|6132g|6132s|6601g|6605c-dvbs2|6610|6610g|6611-dvbs2|6611g|6621|6622-dvbs2|6622-dvbs2_dvbci|6628|generic
                 gx6605s : 6605j|6605s|6616s|combo|generic|isdbt
                 sirius  : 3203|3203H1|3203H2|3213|3213E1|3213E2|3213H1|3213H1-NNE|3213H2|3213S1|3213S2|3215|3215-tvos|6113S1|6113S5|6133S1|6133S5|6613|6613-combo|6613E1|6613E2|6613H1|6613H1-NNE|6613H1-NNE_dvbci|6613H2|6613H2-NNE|6613H2-NNE_dvbci|6613S1|6613S2|6613S5|6613S5-NNE|6613S5-NNE_dvbci|6623H1|6633|6633H1|6633H2|6633-lqfp|generic
                 taurus  : 3113E1|3113E5|3113U1|3113U5|3203U2|3235H1|6601UE|6605H1|6605H1-richepg|6605H5|6605U1|6605U5|6615H1|6617H1|6617H5|6617-NCXSH1A|6617-NNMTH1A|6617-NNMTH5A|6617-NNMTS5A|6617U1|6617U5|6622H1|6622S2|combo|generic
                 gemini  : 6701|6701E|6701H1|6702H1|6702H1-B|6702H1-combo|6702H2|6702H5|6702S5|6702S5-combo|6703X5|generic
                 cygnus  : 6705|6706H1|6706H2-T|6706H5|6706HX-BBT|6706S1|6706S5|6706-T|Cygnus-X5|generic
                 canopus : 3215B|3215B-tvos|6607H1-combo|6607H1-DUAL|6631CH1D|6631CH1D-NNE|6631CHNF|6631SH1D|6631SH1D-NNE|6631SH1D-NNM|6631SH2B|6631SH2D|6631SH5D|6631SHNF|6631SHNG|6631SHXD|6631THNF|fpga|generic
                 vega    : 6631CH1D|6631SH1D|6631SH1D-NNR|6631SH2B|6631SH5D|6631SHNF|6631SHNG|fpga|fpga-NNM|generic|generic-NNM
                 scorpio : 3113X1|3113X5|fpga

  eg: ./build gx3211 6605c-dvbs2 release
      ./build taurus 6617-NNMTS5A release
      ./build cygnus 6706H5 release
      ./build canopus 6631CH1D release
```

可以通过执行 ./build \<chip\> 来查询具体芯片的板级。

```
$ ./build sirius
sirius chip support detail info
3203
3203H1(GX3213-NNECH1/GX3213-DNECH1/GX3213-NNMCH1B/GX3213-DNMCH1B/GX3213-DNECH1BS/GX3213-NNECH1BS)
3203H2(GX3213-NNMCH2B)
3213(GX3213-NNE/GX3213-DNE/GX3213-NNMCHNC/GX3213-DNMCHNC)
3213E1(GX3213E1-NNE/GX3213E1-DNE/GX3213-NNMCE1C/GX3213-DNMCE1C)
3213E2(GX3213E2-NNE/GX3213E2-DNE/GX3213-NNMCE2C/GX3213-DNMCE2C)
3213H1(GX3213H1-NNE/GX3213H1-DNE/GX3213-NNMCH1C/GX3213-DNMCH1C/GX3213H1-NNS)
3213H1-NNE
3213H2(GX3213H2-NNE/GX3213H2-DNE/GX3213-NNMCH2C/GX3213-DNMCH2C)
3213S1(GX3213S1-NNE/GX3213S1-DNE/GX3213-NNMCS1C/GX3213-DNMCS1C)
3213S2(GX3213S2-NNE/GX3213S2-DNE/GX3213-NNMCS2C/GX3213-DNMCS2C)
3215
3215-tvos
6113S1
6113S5
6133S1
6133S5
6613(GX6613-NNE/GX6613-DNE/GX6613-NNMSHNC/GX6613-DNMSHNC/GX6613-NRE/GX6613-DRE/GX6613-NNS/GX6613-NND)
6613-combo
6613E1(GX6613E1-NNE/GX6613E1-DNE/GX6613-NNMSE1C/GX6613-DNMSE1C)
6613E2(GX6613E2-NNE/GX6613E2-DNE/GX6613-NNMSE2C/GX6613-DNMSE2C)
6613H1(GX6613H1-NNE/GX6613H1-DNE/GX6613-NNMSH1C/GX6613-DNMSH1C/GX6613H1-NNES/GX6613H1-NNS)
6613H1-NNE
6613H1-NNE_dvbci
6613H2(GX6613H2-NNE/GX6613H2-DNE/GX6613-NNMSH2C/GX6613-DNMSH2C/S7H2)
6613H2-NNE
6613H2-NNE_dvbci
6613S1(GX6613S1-NNE/GX6613S1-DNE/GX6613-NNMSS1C/GX6613-DNMSS1C)
6613S2(GX6613S2-NNE/GX6613S2-DNE/GX6613-NNMSS2C/GX6613-DNMSS2C)
6613S5(GX6613S5-NNES)
6613S5-NNE
6613S5-NNE_dvbci
6623H1(GX6623H1/GX6623H1-D)
6633
6633H1(GX6613-NNESH1/GX6613-DNESH1/GX6613-NNMSH1B/GX6613-DNMSH1B/GX6613-NNESH1BS/GX6613-DNESS1/GX3213-NNMCH1B)
6633H2
6633-lqfp
generic
```

编译生成的镜像分为spi flash 和nand flash 两种：

- spi flash image
```
output/loader-sflash.bin:	73552
```

- nand flash image
```
output/loader-nand-4K.bin:	151568
output/loader-nand.bin:	118800
output/loader-nand-noecc.bin:	102416
```


## 配置文件

功能配置文件，保存在 conf 目录下：

```
conf/<chip>/<board>/release.config //编译发布 loader 的引导功能配置文件，该配置只开启引导相关的功能。
```

:::{important}
执行./build taurus 6617-NNMTS5A release 命令后，会将 conf/taurus/6617-NNMTS5A/release.config 拷贝成./.config 作为编译配置文件，
如果需要修改默认配置，需要先进行一次编译，然后修改 .config 文件，在执行 make clean;make。
:::

**.config配置说明：**

- 芯片核心配置

| 配置 | 说明  |
| ---- | ----- |
| CONFIG\_ARCH | 平台架构选择，常用结构有CSKY/ARM|
| CHIP\_CORE |芯片核心选择，常用芯片有gx3211/gx6605s/gx3211/gx3113c|
| CHIP\_BOARD | 板级封装选择，不同芯片下有不同的封装|
| CONFIG\_DEMOD |   信道选择，常用信道有dvbs/dvbs2/dvbc/dvbt/dvbt2/dtmb|
| USB\_RESISTANCE |   USB串阻选择，常用串阻有10 //2.2 / 0 Ω|
| ROM\_SERIAL\_BAUDRATE = 107142 |   该配置只在taurus芯片使用24Mhz晶振时，编译.boot的时候加入该配置|
| CONFIG\_24M\_XTAL = y |   配置外部晶振为24Mhz,编译对应的.boot和loader.bin时需要打开该配置|
|DDR配置 |DDR\_TYEP   DDR代数选择，常用代数有1/2/3<br> DDR\_FREQUENCY    DDR频率选择，常用频率有533000000/400000000<br> CONFIG\_SIP   内置DDR选择<br> DDR\_SIZE    DDR配置脚本选择，若有两个不同的DDR参数配置文件ddr1\_162m\_32.h，ddr1\_162m\_64.h，配置的DDR参数不相同，则在.config文件中配置DDR\_SIZE = 32时选择ddr1\_162m\_32.h，配置DDR\_SIZE = 64时选择ddr1\_162m\_64.h|
|CONFIG_CHIP_PACKAGE|封装QFN88/LQFP156/BGA336/LQFP128/LQFP176|
|模块时钟关闭| CONFIG_CLOCK_GP_DISABLE = y<br> CONFIG_CLOCK_GSE_DISABLE = y<br> CONFIG_CLOCK_PIDFILTER_DISABLE = y<br> CONFIG_CLOCK_SDIO_DISABLE = y<br> CONFIG_CLOCK_ETH_DISABLE = y<br> CONFIG_CLOCK_SMARTCARD_DISABLE = n|



- 引导功能配置

| 配置 | 说明  |
| ---- | ----- |
| ENABLE\_SPIFLASH/ENABLE\_SPINAND/ENABLE\_NANDFLASH |引导介质选择配置，只能选择其中一种flash <br> ENABLE\_SPIFLASH： spi nor flash使用配置配置<br> ENABLE\_SPINAND： spi nand flash使用配置配置 <br> ENABLE\_NANDFLASH：nand flash使用配置配置|
| ENABLE_KERNEL_DTB |使用 DTB 信息|
| ENABLE\_UIMAGE |   kernel文件为uImage类型时开启此配置|
| ENABLE\_ROOTFS\_YAFFS2 |   kernel文件为yaffs2类型时开启此配置|
| ENABLE\_ROOTFS\_ROMFS |   kernel文件为romfs类型时开启此配置|
| ENABLE\_ROOTFS\_CRAMFS |   kernel文件为cramfs类型时开启此配置|
| ENABLE\_ROOTFS\_UBIFS |   kernel文件为ubifs类型时开启此配置|
| ENABLE\_CONFIG\_DECOMPRESS\_GZIP |   GZIP解压缩算法|
| ENABLE\_CONFIG\_DECOMPRESS\_LZMA |   LZMA解压缩算法|
| ENABLE\_CONFIG\_DECOMPRESS\_LZO |   LZO解压缩算法|
| ENABLE\_CONFIG\_DECOMPRESS\_ZLIB |   ZLIB解压缩算法|
| CMDLINE\_VALUE |命令行参数，传递给kernel的参数，配置内存大小等，例如CMDLINE\_VALUE为"mem=80M videomem=48M console=ttyS0,115200 init=/init"  详细参考[cmdline](./cmdline.md)说明。|



- 驱动模块配置

| 配置 | 说明  |
| ---- | ----- |
| ENABLE\_IRQ |   中断功能选择|
| ENABLE\_USB |   usb功能选择|
| ENABLE\_GX\_OTP |   chip otp功能选择|
| ENABLE\_NET |   网络功能选择|
| ENABLE\_I2C |   I2C功能选择|
| ENABLE\_GPIO |   GPIO功能选择|
| ENABLE\_IRR |   红外功能选择|
| ENABLE\_RTC |   时钟计时功能选择|
| ENABLE\_CTR |   counter功能选择 <br> ENABLE\_CTR\_CALLBACK <br>   使用counter的定时器回调功能，需要配合ENABLE\_IRQ和ENABLE\_CTR一起使用|
| ENABLE\_CHIP\_INFO |   芯片信息功能选择，例如获取芯片name，Id|
| ENABLE\_WTD |   看门狗功能选择|
| ENABLE\_EEPROM |   eeprom功能选择 <br>   EEPROM\_I2C\_BUS\_NUM：选择使用第几路 <br> I2C EEPROM\_DEVICE\_ADDR：eeprom的器件地址 <br> EEPROM\_TYPE：eeprom的类型|

- 功能模块配置

| 配置 | 说明  |
| ---- | ----- |
| ENABLE\_GDI |   是否使用GDI|
| ENABLE\_LOGO|   通过ENABLE\_LOGO选择是否打开LOGO功能，通过ENABLE\_PAL/ENABLE\_YPBPR\_HDMI\_480I/……等选择打开哪几种制式输出支持。<br>通过logo\_partition\_name指定到哪一个分区查找LOGO数据；通过logo\_file\_name指定logo的文件名，用于到cramfs/romfs格式的分区查找文件时使用；通过g\_cvbs\_mode/g\_ypbpr\_hdmi\_mode设置gxloader启动时的输出制式，枚举定义在include/gx\_api.h中。|
| ENABLE\_SECURE\_VERIFY |   开启lodaer安全启动功能|
| ENABLE\_SECURE\_ALIGN| DDR数据扰乱开关 <br> 一般当otp中对应的bit被写入后，会导致DDR上的数据全部是混乱的，这时需要在.config中添加ENABLE\_SECURE\_ALIGN＝y，这样才能使得访问DDR上的数据是正常的|
| CONFIG\_NO\_GETC/CONFIG\_NO\_PRINY| 串口开关 <br> 在.config文件中添加CONFIG\_NO\_GETC=y时可以关闭串口的接收。<br> 在.config文件中添加CONFIG\_NO\_PRINY=y时可以关闭串口输出。|

- 其他常见配置：

| 配置 | 说明  |
| ---- | ----- |
| ENABLE\_GDB\_DEBUG |   gdb调试loader时开启此配置|
| ENABLE\_FLASH\_TEST |   测试flash功能|
| ENABLE\_MEMORY |   测试内存功能|
| ENABLE\_CMD |   是否开启bootloader的命令功能|
| CONFIG\_BOOTDELAY |   bootloader运行后延时多久进入内核|



## 芯片板级

板级配置文件，保存在 conf 目录下：

```
board/<chip>/<board>/board-init.c //编译发布 loader 的引导板级配置文件。
```

**board-init.c配置说明：**


- 管脚复用


管脚复用是指芯片的一个引脚具有多个功能，但是在一个时刻只能使用其中一个功能，可以通过配置相关寄存器选择引脚功能， 当前 GxLoader 为每个芯片的每个板级提供了管脚复用表，只需要更改一个字段就可以方便的选择管脚的功能，而不需要管相应的寄存器配置。


```
{_RIGHT_, 1, 33, MP_INV_V,MP_INV_V,  0}, //NC |DBGTDI/PORT01(PMUPORT01)
```

1. “//NC |DBGTDI/PORT01(PMUPORT01)” 注释表明这个管脚具有的复用功能，该管脚有两个功能，DBGTDI和PORT01，并且默认第一功能是 DBGTDI，其中 “PMUPORT01” 先不要关心。
2. “}” 前面的数字（上面给的例子是 0）来选择管脚的复用功能，和相应的管脚注释对应起来，当是 0 的时候，即选择管脚的默认第一功能，例子中即DBGTDI；当等于 1 的时候，就是第二个功能，例子中即 PORT01；依次类推，如果注释后面还有第三个功能，可以填 2。
3. PMUPORT01 是指一个 PMU 低功耗芯片（当前是 51）的 IO管脚，它和主 CPU 的几个管脚在物理上是同一个，这里仅仅只是注释方便查看对应关系，当在 PMU 上使用 IO 管脚时，不需要配置管脚复用。


- GPIO配置

对 g\_gpio\_table 表进行配置， 其中结构体说明如下

| 配置 | 说明  |
| ---- | ----- |
|vir\_gpio|虚拟gpio管脚|
|phy\_gpio|物理 gpio 管脚，应用中将统一操作虚拟 gpio，物理 gpio 不可见，也可将 vir\_gpio 和 phy\_gpio 设置为相等|
|config\_valid|配置该管脚是否有效，一般设置为有效 GX\_GPIO\_CONFIG\_VALID|
|io\_mode|设置默认是输入/输出状态|
|output\_value|在默认为输出状态时生效，设置默认输出高电平/低电平|

- TS功能配置

Demux模块支持4路输入源，其中前3路输入源会有一路固定来自内置并行Demod，其余几路可以来自外置并行Demod、外置串行Demod， 另外部分芯片还支持来自内置T2MI。最后一路输入源固定来自CPU，即CPU可以通过接口把内存中的数据输入给Demux，详细参考[ts_config](ts_config/ts_config.md)。
