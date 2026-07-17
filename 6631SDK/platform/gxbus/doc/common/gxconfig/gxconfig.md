# GX Config文件格式

GX Config 文件是一种 .ini 文件，.ini 文件是 Initialization File 的缩写，即初始化文件，采用一种 key-value 格式描述系统初始化的各种配置。

GX Config 文件作为一种 .ini 标准文件，用以描述国芯微数字多媒体设备初始化配置信息。

文件可以分为多个“节[Section]”，每个“节”以方括弧表示，如： [system]。每个“节”以下，可以有多个 key-value 不重复对应字段。

文件注释采用标准 .ini 文件注释标识，即 "#" 或 ";"。


# GX Config接口使用范例

## config.ini文件范例

```ini
[system]
version=0.0.1
update_type= auto
logo_version=0.0.1
application_version=0.0.1
rootfs_version=0.0.1
datafs_version=0.0.1

[ui]
gui_mem_pool=12M
hw_mem_pool=1M
schedule_time=60

[application]
dmx_pid=0x1CA
dmx_id=0
dmx_source=DEMUX_TS1
fe_init_param=|1:0:0xe4:41:0:0xc6:&1:1:2:0:0:1
fe_modulation=QPSK
fe_symbolrate=27500000
fe_frequency=1150000
fe_voltage=18V
fe_tone=ON
fe_diseqc10=PORT4
```

## 获取设置范例

当使用如： GxBus_ConfigGet 或 GxBus_ConfigSet 接口，获取或设置参数时，“节[Section]”内容与关键字内容采用“>”分隔，如
system 节中的 version 关键字，表示为： "system>version" 传入接口。

## 注意事项

由于传入接口的参数“节[Section]”内容与关键字内容采用“>”分隔，故节与关键字内容中不得出现“>”。

ini 文件本质可以节嵌套多级，但国芯微接口只支持一级。

