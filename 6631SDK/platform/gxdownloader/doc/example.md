# 客户集成

## 编译

请查看[功能特性](./feature.md)中《配置和构建》章节。


## 分区表更改

为了支持 GxDownloader，需要有选择的增加三个分区，DLP 分区、备份 DLP 分区 和 OTA 分区，其中 DLP 分区是必须增加的。

**DLP 分区:** 支持 GxDownloader 时，必须包含该分区，存放了 DLP 信息。工程模式和方案模式都需要操作该分区。

**备份 DLP 分区:** 该分区的目的是为了对 DLP 分区进行备份，防止 DLP 分区数据在被写入的时候断电而丢失。

**OTA 分区:** 该分区的地址和大小必须分别和 OTA_FORCE_FLASH_ADDR 和 OTA_FORCE_FLASH_SIZE 一致。当支持 GxDownloader 工程模式时，支持工程模式时该分区必须存在，用于存放编译生成的  ota.img。如果不需要工程模式，该分区不需要。

## 应用开发

方案模式是基于 GxDownloader 编译生成的库进行二次开发。GxDownloader 编译生成 libdownlader.a，同时提供 gxdlp_api.h，gxota_api.h，gxota_msg.h 三个头文件。

API 接口功能：
- 初始化和读写 DLP 分区接口
- 设置各种升级模式标志
- 设置升级参数(网络升级需要的IP地址，OTA 升级需要的前端配置等参数)
- OTA 升级触发条件检测
- OTA 升级功能(后续支持 USB/NET)

- OTA 升级过程消息交互接口

函数接口:

- \ref GxDLP_Init  DLP 分区初始化，把 DLP 数据加载到内存，必须先调用此函数后，再调用其他DLP分区接口函数
- \ref GxDLP_Sync  DLP 同步，把 DLP 更改了的数据写到 Flash
- \ref GxDLP_Close  关闭 DLP
- \ref GxOTA_Start_Detect_Version  开启检测 OTA 新版本检测线程
- \ref GxOTA_Stop_Detect_Version  关闭检测 OTA 新版本检测线程
- \ref GxOTA_isTrigger  判断 OTA 已经检测到新版本
- \ref GxOTA_SetTrigger  当检测到 OTA 有新版本时，设置 OTA 要使用的配置参数以便工程模式使用。
- \ref GxOTA_Put_Msg  发送升级过程消息，包括升级进度百分比，升级状态
- \ref GxOTA_Get_Msg  获取升级过程消息，包括升级进度百分比，升级状态
- \ref GxOTA_Start_Upgrade  开启升级线程
- \ref GxOTA_Stop_Upgrade  关闭升级线程
- \ref GxOTA_Start_Write_Flash 通知升级线程可以把升级数据写到 Flash 中，这个目的是因为升级线程在接收完升级包后，不会立马把升级包写到 Flash (目的是为了升级线程和上层 GUI 界面的同步)，需要调用该函数给升级线程发消息，升级线程收到消息后才会做写 Flash 的操作。

功能集成实例:

根据分区表，加载 DLP 分区和备份 DLP 分区存储的信息，因为 GxDownloader 的所有功能都必须根据 DLP 分区存储的信息进行合适的操作。

```
GxDLP_Init("OEM", "BACKOEM"); //"OEM" 为实际的 DLP 分区名字，"BACKOEM" 为实际的备份 DLP 分区名字
```

在方案中前端锁定后，开启版本检测功能，假设已经知道升级TS流的PID是0x7d3。 

```
GxOTA_Start_Detect_Version(0, 0, 0x7d3); //dmxid == 0, tsid == 0, pid == 0x7d3 开启OTA版本检测线程
while (GxOTA_isTrigger() == false)   //每100ms循环判断OTA是否检测到新版本
	GxCore_ThreadDelay(100);
```

当检测到新版本后可以选择设置配置参数，断电重启会进入工程模式升级。

```
	ota_trigger_info ota_info = {0};
	ota_info.OTA_Type = GXDLP_OTA_DDTEX_DOWNLOAD;
	ota_info.ota_u.ddtex_upgrade.dmx_pid = pid;
#if DEMOD_DVB_C
	ota_info.ota_u.ddtex_upgrade.Frequency = previous_dvbc_ota_param.fre;
	ota_info.ota_u.ddtex_upgrade.Symbolrate = previous_dvbc_ota_param.symbol_rate;
	ota_info.ota_u.ddtex_upgrade.Modulation = previous_dvbc_ota_param.qam+3;
#elif DEMOD_DTMB
	ota_info.ota_u.ddtex_upgrade.Frequency = previous_dtmb_ota_param.fre;
	ota_info.ota_u.ddtex_upgrade.bandwidth = BANDWIDTH_8_MHZ;
#else
	uint32_t polar;
	ota_info.ota_u.ddtex_upgrade.Frequency = app_show_to_tp_freq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data);
	ota_info.ota_u.ddtex_upgrade.Symbolrate = s_CurOtaTp.tp_data.tp_s.symbol_rate;
	polar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
	if(polar == SEC_VOLTAGE_ALL)
	{
		ota_info.ota_u.ddtex_upgrade.Polarity = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
	}
	else
	{
		ota_info.ota_u.ddtex_upgrade.Polarity = polar;
	}
	ota_info.ota_u.ddtex_upgrade.Modulation = DVBS_QPSK_12;
	ota_info.ota_u.ddtex_upgrade.Sat22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);
	ota_info.ota_u.ddtex_upgrade.Diseqc10_Port=s_CurOtaSat.sat_data.sat_s.diseqc10;
	ota_info.ota_u.ddtex_upgrade.Diseqc11_Port=s_CurOtaSat.sat_data.sat_s.diseqc11;
#endif
	GxOTA_SetTrigger(&ota_info);
```

在检测到新版本后，也可以选择立即升级，不用进入工程模式升级。

```
GxOTA_Start_Upgrade(0, 0, 0x7d3); //dmxid == 0, tsid == 0, pid == 0x7d3 开启OTA升级
struct GxOTA_Msg ota_msg={0};
while(1) {
	OTA_Get_Msg(&ota_msg);
	switch(ota_msg.type):
		case GXOTA_GUI_PROCESS: //OTA升级进程百分比
			//可以添加GUI显示函数显示进度条
			break;
		case GXOTA_GUI_STATUS: //OTA 升级状态 {
		    //可以根据升级状态在界面上显示提示
		    switch(ota_msg.msg_id) {
		    	case GXOTA_STATUS_VERSION_OK: //表示OTA已经得到完整的数据包，并且版本校验正确,这里要添加函数GxOTA_Start_Write_Flash()通知底层可以把升级包写到Flash中。
		    	    //添加写Flash之前必要的操作，因为在写Flash的速度很慢，其他操作会卡住，比如界面响应
		    		GxOTA_Start_Write_Flash(); //通知底层线程开始写Flash
		    }
		    break;
		}
	GxCore_ThreadDelay(100);
}
```

DLP 存储的信息可以见 [DLP分区](feature.md#dlp分区) 一章，相关函数也列出。下面列出 DLP 写操作的例子。

```
GxDLP_Set_SERVER_IP_ADDR("192.168.110.67"); //设置 TFTP 服务器 IP 地址
GxDLP_Set_HOST_IP_ADDR("192.168.110.68"); //设置 机顶盒的 IP 地址
GxDLP_Sync();  //调用 GxDLP_Sync() 才会把更改后的数据写到 Flash 中，否则前面设置数据只在内存中，断电或者重启都会丢失。 GxDLP_Sync() 函数不要重复调用，因为每一次调用都是一次写 Flash操作，写Flash速度是很慢的。并且进程会等待写Flash操作完成，而不做其他事。
```

## 升级镜像和升级流制作

升级镜像和码流制作流程请跳转到 [升级码流制作工具](./tools.md)


