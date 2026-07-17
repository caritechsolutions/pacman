## DLP 分区

DLP 是 RAW 格式的分区，DLP 是 Downloader Partition 的缩写，用于方案和 GxDownloader 信息交互的分区。GxDownloader 提供 gxdlp_api.h 的 API 接口来读写分区的内容，能够实现方案和 GxDownloader 使用一套接口实现信息交互。
任何 RAW 格式的分区都可以通过 int32_t GxDLP_Init(char *dlp,char *backup_dlp); 接口格式化成为 DLP 分区。
系统分区 DLP 通过 Flash.conf 配置，genFlash 生成。GxDownloader 使用时必须需要 DLP 分区存在。

DLP 分区就是文本文件，记录了设备的 OEM 信息(即厂商 ID，硬件版本号，软件版本号等信息)和升级需要使用的信息。一个典型的 DLP 文本文件如下所示:

```
[variable_oem]
version=0.0.1
[system]
hardware_version=00.00.00.00
manufacture_id = 00.00
[software]
application_version=00.00.00.00
application_update_version=00.00.00.00
[update]
type=BOOT
upgrade_patch_file_name=update.bin
fe_tuner_type=TUNER_RDA5815M
fe_demod_type=DEMOD_Taurus_DVBS
dmx_id=0
dmx_pid=2003
dmx_source=0
dmx_timeout=10000
fe_type=DVB_S
fe_modulation=DVBC_16QAM
fe_symbolrate=27500
fe_frequency=1150000
fe_polar=13V
fe_22k=ON
fe_diseqc10_port=0
fe_diseqc11_port=0
fe_repeat_times=0x3000
ota_repeat_times=3
force_upgrade_flag=0
fe_tuner_dev_id=1
fe_tuner_i2c_addr=0x19
fe_tuner_osc_fre=16
fe_demod_dev_id=2
fe_demod_ts_mode=PARALLEL
fe_demod_iq_mode=1
fe_demod_is_oscillator=1
fe_demod_ts_pin=1
fe_polar_gpio=12
fe_lnb_invert=1
partition_status=0

```

可以看到每个字段就是"name=value"的简单形式，name=value中间不要有空格.下面解释下目前用到的每个字段的含义，字段对应value的值和操作该字段对应的 DLP 函数。

### DLP head节

**[variable_oem]   (必选)**

说明： DLP 文件的开头，且必须放在文件头部，否则 DLP 操作函数认为 DLP 分区数据错误

**version (可选)**

说明:  DLP 版本号，目前不用

值:  随意

例子: version=0.0.1

### 系统信息节

**[system]  (必选)**

说明:  system 节的开头，该节包括硬件版本号和厂商 ID 字段

**hardware_version  (必选)**

说明:  设备硬件版本号，在 [system] 节里，升级时会把接收到的升级包包含的硬件版本号和设备的硬件版本号做对比，相等才能升级。

值: 格式 xx.xx.xx.xx，每个x是16进制的数字

例子: hardware_version=00.00.00.01

函数: [**GxDLP\_Get\_HardwareVersion**](group__label_1gae7d43be41421701c37b5b8384fe9f28e.md)

**manufacture_id   (必选)**

说明:  设备厂商 ID，在 [system] 节里，升级时会把接收到的升级包包含的厂商 ID 和设备的厂商 ID 做对比，相等才能升级。

值: 格式 xx.xx，每个x是16进制的数字

例子: manufacture_id=00.01

函数: [**GxDLP\_Get\_ManufactureId**](group__label_1gaa968937f994f68ecc4e9d6ee3950855d.md)

### 软件信息节

**[software] (必选)**

说明:  software 节的开头，该节包括软件版本号和升级软件版本号字段

**application_version (必选)**

说明:  设备软件版本号，在 [software] 节，升级时会把接收到的升级包包含的软件版本号和设备的软件版本号做对比，默认升级包软件版本号大于设备软件版本号才能升级。

值: 格式 xx.xx.xx.xx，每个x是16进制的数字

例子: application_version=00.00.00.00

函数: [**GxDLP\_Get\_SoftwareVersion**](group__label_1ga34cf1cf78d644ca7f12b2edba3a7b35a.md) , [**GxDLP\_Set\_SoftwareVersion**](group__label_1gaa9a3efbd4a09b13c4ab93331bc994317.md)

**application_update_version (必选)**

说明:  升级包软件版本号，在 [software] 节，临时存储升级包软件版本号。当升级完成后，把此字段的值更新到application_version 字段，即更新设备软件版本号。

值: 格式 xx.xx.xx.xx，每个x是16进制的数字, 初值设为00.00.00.00即可。

例子: application_update_version=00.00.00.00

函数: [**GxDLP\_Get\_UpdateSoftVersion**](group__label_1gacdcf58bf34a67a44170087cfbf7a6a4b.md) , [**GxDLP\_Set\_UpdateSoftVersion**](group__label_1ga2d6b5044f943b1ef08d36d08c0792ee0.md)

**language (可选)**

说明:  升级界面语言选择，在 [software] 节，如果 DLP 文件不存在此字段，默认语言选择英语

值: Chinese , English

例子: language=Chinese

函数: [**GxDLP\_Get\_Language**](group__label_1ga19fea6769e39db1af97c2927d428960a.md) , [**GxDLP\_Set\_Language**](group__label_1ga09898f7529bf474c2b05259583f48857.md) 

### 升级信息节

**[update] (必选)**

说明: update 节的开头，该节包含升级相关的配置参数

**type (必选)**

说明: 升级类型

值: 

BOOT: 不升级，如果 type=BOOT, 开机正常跳转到应用程序

OTA_DDTEX: DDTEX 升级

USB: USB升级

NET; 网络 TFTP 升级

例子: type=OTA_DDTEX

函数: [**GxDLP\_Get\_UpdateType**](group__label_1ga39ce51f25187fe3193855303e0e67277.md) , [**GxDLP\_Set\_UpdateType**](group__label_1ga47fa5a0bd87aed25269d3d6852722dca.md)

**ota_repeat_times (必选)**

说明:  升级失败重试次数，用于 loaderOTA。当 type 设置升级，设备启动会进入升级，但是会遇到升级TS流接收失败或者升级文件有问题等导致升级失败，防止设备一直处于升级状态，当升级失败次数超过 ota_repeat_times后，自动设置 type=BOOT，设备重启后进入正常启动流程

值：10进制或者16进制数值

例子: ota_repeat_times=3

函数: [**GxDLP\_Get\_OtaRepeatTimes**](group__label_1gab8a43ee78f3aafbec089d9b4bf6bf374.md) , [**GxDLP\_Set\_OtaRepeatTimes**](group__label_1gad3e9b4ec25ad22568f7908752944da9e.md)

**upgrade_patch_file_name (必选)**

说明: 升级镜像包文件名,  制作的升级镜像包文件名必须和 upgrade_patch_file_name 定义的一致，DSM-CC 升级时会对比接收到的升级镜像包文件名和 upgrade_patch_file_name 定义的是否一致，USB 升级或者 NET 升级指定 upgrade_patch_file_name 定义的文件名去查找升级镜像包

值: 符合 UNIX 标准定义的文件名

例子: upgrade_patch_file_name=update.bin

函数: [**GxDLP\_Get\_UpgradeFileName**](group__label_1ga27a8ceb16a24435fdb5710b37fe1c612.md) , [**GxDLP\_Set\_UpgradeFileName**](group__label_1ga55ec84c5ada1adbfe38940a5fa6c8092.md)

**dmx_id (必选)**

说明: demux 选择

值：10 进制或者16 进制数值

例子: dmx_id=0

函数: [**GxDLP\_Get\_DmxId**](group__label_1ga8d14f3a727749b76dec8265de3a38445.md) , [**GxDLP\_Set\_Dmxid**](group__label_1ga3ec3bb1c0120e95d7bf326862bfcce11.md)

**dmx_pid (必选)**

说明:  升级 TS 流的 PID

值：10 进制或者 16 进制数值

例子: dmx_pid=2003

函数: [**GxDLP\_Get\_DmxPid**](group__label_1gabd62d0c6a11713009380c6f1781002a7.md) , [**GxDLP\_Set\_DmxPid**](group__label_1ga4f33f0253543cde2e9d05661aec8979c.md)

**dmx_source (必选)**

说明： TS 流输入源 ID

值: 10 进制或者 16 进制数值

例子: dmx_source=0

函数: [**GxDLP\_Get\_DmxSource**](group__label_1gaa4a984f76a35a00609bac708af591e6f.md) , [**GxDLP\_Set\_DmxSource**](group__label_1ga57ed49de38e2c83281bc219fead0215a.md)

**dmx_timeout (必选)**

说明： demux 过滤数据超时时间，指 demux 在 dmx_timeout 设置的超时时间内过滤不到升级 TS 流数据，就不在过滤，提示升级失败

值: 10 进制或者 16 进制数值, 单位: 秒, 建议值 30 秒 ~ 60 秒

例子: dmx_timeout=30

函数: [**GxDLP\_Get\_DmxTimeout**](group__label_1ga78677f241410eb638c0fbc853595358a.md) , [**GxDLP\_Set\_DmxTimeout**](group__label_1ga33459dd3cd12be632aa28aadda424541.md)

**fe_tuner_type (必选)**

说明: tuner 类型选择

值:  TUNER_AV2018, TUNER_AV2020, TUNER_MXL608_DTMB, TUNER_MXL608_T, TUNER_MXL608_C, TUNER_TDA18250, TUNER_TDA18250A, TUNER_TDA18273_DTMB, TUNER_TDA18273_DVBC, TUNER_R836_T, TUNER_R836_C, TUNER_R910_T, TUNER_R910_C, TUNER_SI2141_T, TUNER_SI2141_C, TUNER_R848_S, TUNER_R848_C, TUNER_R848_T, TUNER_R848_DTMB, TUNER_RDA5815M, TUNER_ATBM2040, TUNER_ATBM2030, TUNER_SHARP7306, TUNER_R836_DTMB

例子: fe_tuner_type=TUNER_RDA5815M

函数: [**GxDLP\_Get\_FeTunerType**](group__label_1ga34c22fa8a960ea2704f1a2ae6feade8c.md) , [**GxDLP\_Set\_FeTunerType**](group__label_1ga818b160947de78cd78c7df767b19eec9.md)

**fe_tuner_dev_id (必选)**

说明: tuner 的设备 ID 设置

值: 10 进制或者 16 进制数值,典型值 0,1,2，目前机顶盒最多3个 tuner

例子: fe_tuner_dev_id=0

函数: [**GxDLP\_Get\_FeTunerDevId**](group__label_1ga70a6002579e7680597a0f34b633f95e3.md) , [**GxDLP\_Set\_FeTunerDevId**](group__label_1gabf8c3091e4faa36b71b2902e0c1f018b.md)

**fe_tuner_i2c_addr (可选)**

说明: tuner i2c 地址设置, 如果 DLP 文件没有此参数，使用 fe_tuner_type 选择的 tuner 类型默认的 i2c 地址

值: 10 进制或者 16 进制数值

例子: fe_tuner_i2c_addr=0xc0

函数: [**GxDLP\_Get\_FeTunerI2cAddr**](group__label_1ga16774ab400ab101d5d7d41b431aa8e80.md) , [**GxDLP\_Set\_FeTunerI2cAddr**](group__label_1ga5abf7e131f00c47d5c525423521f2ef4.md)

**fe_demod_type (必选)**

说明: demod类型选择

值:  DEMOD_GX1121D, DEMOD_GX1503B_DTMB, DEMOD_GX113X, DEMOD_GX3211, DEMOD_GX1001, DEMOD_GX1503_DVBC, DEMOD_GX1503_DTMB, DEMOD_GX1801, DEMOD_ATBM78X, DEMOD_ATBM888X_DVBC, DEMOD_ATBM888X_DTMB, DEMOD_ATBM783X_DVBC, DEMOD_ATBM783X_DVBS, DEMOD_GX1133, DEMOD_Sirius_DVBS, DEMOD_Sirius_DVBC, DEMOD_MXL683, DEMOD_Taurus_DVBS, DEMOD_Taurus_DVBC, DEMOD_GX6605S_DVBS

例子: fe_demod_type=DEMOD_Taurus_DVBS

函数: [**GxDLP\_Get\_FeDemodType**](group__label_1ga2a9615b22f1937a4fff52b199669b8aa.md) , [**GxDLP\_Set\_FeDemodType**](group__label_1ga37152cdc8a7d54a4c2acc79be46c73b5.md)

**fe_demod_dev_id (必选)**

说明: demod的设备 ID 设置

值: 10 进制或者 16 进制数值,典型值 0,1,2，目前机顶盒最多3个 demod

例子: fe_demod_dev_id=0

函数: [**GxDLP\_Get\_FeDemodDevId**](group__label_1ga8b4b54ee9e7b48ae05a2c80a635b8c30.md) , [**GxDLP\_Set\_FeDemodDevId**](group__label_1ga90b16483204663607078df2f80381b71.md)

**fe_demod_i2c_addr (可选)**

说明: demod i2c 地址设置, 如果 DLP 文件没有此参数，使用 fe_demod_type 选择的 demod 类型默认的 i2c 地址

值: 10 进制或者 16 进制数值

例子: fe_demod_i2c_addr=0xa4

函数: [**GxDLP\_Get\_FeDemodI2cAddr**](group__label_1ga195f3fd541f27125cf32aeb8203e271a.md) , [**GxDLP\_Set\_FeDemodI2cAddr**](group__label_1ga81f396e41c5e55167490adceebaa481c.md)                                                              

**fe_repeat_times (必选)**

说明： 前端 tuner 的锁定超时时间，指前端在 fe_repeat_times 设置的超时时间内不能锁定 TS 信号，提示升级失败。前端会每 1 秒钟，查询 10 次锁定状态

值: 10 进制或者 16 进制数值, 单位: 秒,  建议值 30 秒~ 60 秒

例子: fe_repeat_times=30

函数:  [**GxDLP\_Get\_FeRepeatTimes**](group__label_1ga787db71d762dfb2ec06594e808a5f690.md) , [**GxDLP\_Set\_FeRepeatTimes**](group__label_1gaa73af6fcd858d82052fc730894bfdca8.md)

**fe_type (必选)**

说明: 前端 tuner 的类型设置，与机顶盒的前端类型匹配

值: DVB_S, DVB_S2, DVB_C, DVB_C2, DVB_T, DVB_T2, DTMB, ATSC_T, ATSC_C, ABS_S, DIRECTV, TSDB_T, J83B

例子: fe_type=DVB_S2

函数:  [**GxDLP\_Get\_FeType**](group__label_1ga6fdcf8db32b0a93b2def0d7e427b9def.md)  ,  [**GxDLP\_Set\_FeType**](group__label_1ga7ea409e5fdd67da1f663916a008d0372.md)

**fe_symbolrate (必选)**

说明:  前端 tuner 的符号率，单位: Kbps

值: 10 进制或者 16 进制数值

例子: fe_symbolrate=27500

函数: [**GxDLP\_Get\_FeSymbolrate**](group__label_1ga3a8666e372622cf2333041b63dc6624b.md) , [**GxDLP\_Set\_FeSymbolrate**](group__label_1gafe6b633ccb5cde37b8a069588556c0cc.md)

**fe_frequency (必选)**

说明:  前端 tuner 的频率，单位: KHz

值: 10 进制或者 16 进制数值

例子: fe_frequency=1150000

函数: [**GxDLP\_Get\_FeFrequency**](group__label_1ga15a55a99da09c20e34f0a099cddc8aa5.md) ,  [**GxDLP\_Set\_FeFrequency**](group__label_1gabaebfad06ad7404f313d1692545296a7.md)

**fe_modulation (必选)**

说明:  前端 tuner 的调制选择，用于 DVBC

值: 必须设置以下列出的值 DVBC_16QAM, DVBC_32QAM, DVBC_64QAM, DVBC_128QAM, DVBC_256QAM

例子: fe_modulation=DVBC_16QAM

函数: [**GxDLP\_Get\_FeModulation**](group__label_1gae1aaeb1dfc4cb5f49d1f471f8b757c55.md) , [**GxDLP\_Set\_FeModulation**](group__label_1gab1e7d0f59fe45a39f1a585b4487dc1ca.md)

**fe_bandwidth (必选)**

说明:  前端 tuner 的 bandwidth 选择

值: BANDWIDTH_8_MHZ, BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ

例子: fe_bandwidth=BANDWIDTH_8_MHZ

函数: [**GxDLP\_Get\_FeBandwidth**](group__label_1gad3f505f9f0a11c5331e105b71c783a50.md) , [**GxDLP\_Set\_FeBandwidth**](group__label_1gab1843823261f0a1884b583b86a11eac7.md)

**fe_polar (必选)**

说明: 前端 tuner 的 polar 选择

值: 13V, 18V, OFF

例子: fe_polar=13V

函数: [**GxDLP\_Get\_FePolarity**](group__label_1gad5ccf8e5696471390b53ad1ef323f2b9.md) ,  [**GxDLP\_Set\_FePolarity**](group__label_1ga1d83c0361263cc31a35ca9e0c56cf098.md)

**fe_polar_invert (必选)**

说明: 前端 tuner 的 polar invert 设置

值: 0或者1

例子: fe_polar_invert=1

函数: [**GxDLP\_Get\_FePolarInvert**](group__label_1gacca9ceeec4ab2f3a4c795b53b8ac2584.md) , [**GxDLP\_Set\_FePolarInvert**](group__label_1ga99a32f5a80b1972ed46e0efe1775c76c.md)

**fe_polar_gpio (必选)**

说明: 前端 tuner 的 polar gpio 设置

值: 10 进制或者 16 进制数值

例子: fe_polar_gpio=1

函数: [**GxDLP\_Get\_FePolarGpio**](group__label_1ga1926b50c515977d2fac336df12dd4835.md) , [**GxDLP\_Set\_FePolarGpio**](group__label_1ga6b3fc048a75786d539f357dba0242f81.md)

**fe_lnb_invert (必选)**

说明: 前端 tuner 的 lnb invert 选择设置

值: 10 进制或者 16 进制数值

例子: fe_lnb_invert=1

函数: [**GxDLP\_Get\_FeLnbInvert**](group__label_1ga2390ab69ed61e832fa517d6b33687b9a.md) , [**GxDLP\_Set\_FeLnbInvert**](group__label_1ga2c535fb73d1f19258ccd7a2e5e6785e3.md)

**fe_22k (必选)**

说明: 前端 tuner 的 22k 选择

值: ON, OFF

例子: fe_22k=ON

函数: [**GxDLP\_Get\_FeSat22k**](group__label_1ga4edd509faf4bfe7c606ba07832d0e248.md) , [**GxDLP\_Set\_FeSat22k**](group__label_1ga3d803dd8e300939e68cce53f265f021c.md)

**fe_diseqc10_port (必选)**

说明: 前端 tuner 的 diseqc10_port 选择

值: 10 进制或者 16 进制数值

例子:  fe_diseqc10_port=0

函数: [**GxDLP\_Get\_FeDiseqc10\_Port**](group__label_1gaf5e6de0bcc30b3cecd4468c232116636.md) , [**GxDLP\_Set\_FeDiseqc10\_Port**](group__label_1gaf4bacb0013efb8ab59a2dbff73dd0559.md)

**fe_diseqc11_port (必选)**

说明: 前端 tuner 的 diseqc11_port 选择

值: 10 进制或者 16 进制数值

例子:  fe_diseqc11_port=0

函数: [**GxDLP\_Get\_FeDiseqc11\_Port**](group__label_1ga3066fe10230b14b96318edbcda98b8d4.md) , [**GxDLP\_Set\_FeDiseqc11\_Port**](group__label_1ga972d290b71c5df1cf8be9bc65d4412a5.md)

**fe_demod_ts_mode (必选)**

说明: 前端 demod 的 ts mode 选择, 选择串行，并行

值: PARALLEL, SERIAL, OTHER

例子:  fe_ts_mode=PARALLEL

函数: [**GxDLP\_Get\_FeDemodTsMode**](group__label_1ga18b8e73bebc88e73908040b41df1bb1b.md) , [**GxDLP\_Set\_FeDemodTsMode**](group__label_1ga9dc508737d6b91d1f9b2a78309bba1d3.md)

**fe_demod_iq_mode (可选)**

说明: 前端 demod 的 iq_mode  选择, 选择串行，并行

值:  10 进制或者 16 进制数值

例子:  fe_demod_iq_mode=1

函数: [**GxDLP\_Get\_FeDemodIqMode**](group__label_1gac36ed258588255fc760c356296b3c894.md) , [**GxDLP\_Set\_FeDemodIqMode**](group__label_1gaa24767a30f05847c70eaa6b010d98949.md)

**fe_demod_is_oscillator (可选)**

说明: 前端 demod 的 is_oscillator 设置

值: 0 或者 1

例子:  fe_demod_is_oscillator=0

函数: [**GxDLP\_Get\_FeDemodIsOscillator**](group__label_1gafc771b9f883c3bfa40778c75938c6113.md) , [**GxDLP\_Set\_FeDemodIsOscillator**](group__label_1ga0f1e85817916caabfe799925eed55353.md)

**fe_demod_ts_pin (可选)**

说明: 前端 demod 的 ts_pin 设置

值: 10 进制或者 16 进制数值

例子:  fe_demod_ts_pin=0

函数: [**GxDLP\_Get\_FeDemodTsPin**](group__label_1ga8b49624739c7324a4ca01df7b2177d5f.md) , [**GxDLP\_Set\_FeDemodTsPin**](group__label_1ga00423eacd5d53b363b012822ca2f4212.md)

**tftp_server_ip_addr(必选)**

说明: TFTP 网络升级时, TFTP 服务器的 IP 地址

值: IP 地址格式

例子: tftp_server_ip_addr=192.168.110.67

函数: [**GxDLP\_Set\_SERVER\_IP\_ADDR**](group__label_1ga0c6e3dbbe1d25e23b149068238535bf1.md) , [**GxDLP\_Get\_SERVER\_IP\_ADDR**](group__label_1gaf51bfa5b1aa4b8472f5df498dac6b9e5.md)

**tftp_port(必选)**

说明: TFTP 升级时，TFTP 端口值，默认标准 TFTP 端口是 69

值: 10 进制或者 16 进制数值

例子: tftp_port=69

函数: [**GxDLP\_Set\_TFTP\_PORT**](group__label_1gabcb2f89db77cedaa2dccd61e7bdc1d15.md) , [**GxDLP\_Get\_TFTP\_PORT**](group__label_1ga2b39e8ef8e07ebeaf859774d31d755ea.md)

**tftp_host_ip_addr(必选)**

说明: TFTP 升级时，机顶盒设备的 IP 地址，由于 TFTP 是基于局域网传输的，所以 tftp_host_ip_addr 和tftp_server_ip_addr 应该设置为同一个网段

值: IP 地址格式

例子: tftp_host_ip_addr=192.168.110.68

函数:[**GxDLP\_Get\_HOST\_IP\_ADDR**](group__label_1ga860cf631d63ac401a439ada59cde18d2.md)  , [**GxDLP\_Set\_HOST\_IP\_ADDR**](group__label_1ga8719c01100ff4c6759a5f9bb2015e1b5.md) 

**force_upgrade_flag(必选)**

说明: 用于 OTA 升级时忽略版本号, 当 OTA 升级时，收到升级流后，不会比较 application_version 和application_update_version 的值，直接解析升级流的数据进行升级。主要用于调试使用，实际使用请设置为0

值: 10 进制或者 16 进制数值

例子: force_upgrade_flag=0

函数: [**GxDLP\_Get\_ForceUpgradeFLag**](group__label_1ga3f3e047022e5b09df398fc8c42468e9b.md) , [**GxDLP\_Set\_ForceUpgradeFlag**](group__label_1gaa6641db2a179d6c08c172faf04ef3e62.md)

**partition_status(必选)**

说明: partition_status 用于记录每个分区的状态。初始值一定要设为 0. 在升级某个分区的时候，先记录此分区为升级状态，完成后，再标记完正常。目的是为了在升级某个分区的时候，出现断电，下次重启时，可以帮用户指出具体升级失败的分区，然后针对对应分区重新升级

值: 10 进制或者 16 进制数值

例子: partition_status=0

函数: [**GxDLP\_Get\_PartitionStatus**](group__label_1ga948bf9a96ad57481bcb15f061cd26332.md) , [**GxDLP\_Set\_PartitionStatus**](group__label_1gabc7e3aaa68c656abc00690913f721c04.md)

