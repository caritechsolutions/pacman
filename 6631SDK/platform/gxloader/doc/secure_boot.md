# 安全启动

gxloader 实现了[安全架构](../../gxsecure/doc/basis/SystermArchitecture.md#安全架构)中[安全启动](../../gxsecure/doc/basis/SystermArchitecture.md#安全启动)部分的 [签名验证](../../gxsecure/doc/basis/SystermArchitecture.md#签名验证)、 [镜像解密](../../gxsecure/doc/basis/SystermArchitecture.md#镜像解密)、 [细分市场](../../gxsecure/doc/basis/SystermArchitecture.md#细分市场)、 [版本控制](../../gxsecure/doc/basis/SystermArchitecture.md#版本控制)、 [完整性检查](../../gxsecure/doc/basis/SystermArchitecture.md#完整性检查)、 [安全环境](../../gxsecure/doc/basis/SystermArchitecture.md#安全环境) 的功能。

:::{important}
具体 CA 的安全启动，参考 CA 小组发布的集成文档。
:::

## 签名验证

ROM 根据[OTP](../../gxapi/doc/secure/fuse/feature.md#签名验证) 签名验证配置，启动验签流程。

- 一级验签

- 二级验签




## 镜像解密


ROM 根据[OTP](../../gxapi/doc/secure/fuse/feature.md#镜像解密) 镜像解密配置，启动解密流程。

:::{important}
ROM 的逻辑是先解密在验签。
:::

支持派生和非派生解密两种模式，流程如下：

- 非派生模式

- 派生模式

## 细分市场

ROM 根据[OTP](../../gxapi/doc/secure/fuse/feature.md#签名验证) 签名验证配置，启动验签流程。ROM会启动 Market ID 检测功能。

- market id 市场校验码，用于表示不同的厂家
- market id 总共 4 个字节，存储在 OTP 中，芯片开启安全启动后，开机会把 OTP 中的 market id 与 Flash 中的 market id 比较，不一样，启动异常，Flash 中的 loader.bin 是非安全的或者被篡改。
- 芯片 OTP 中存储的 market id 与 Flash 中的 market_id 的字节序要一致，比如 OTP 地址存放 market id  0x100 ~ 0x103 的分别是 0x4B，0x41，0x52，0x4B，那 Flash 地址存放 market_id 的地址 0x1000 ~ 0x1003 也应该是 0x4B，0x41，0x52，0x4B。


## 版本控制
ROM 根据[OTP](../../gxapi/doc/secure/fuse/feature.md#签名验证) 签名验证配置，启动验签流程。ROM会启动 版本控制 检测功能。

## 完整性检查

## 安全环境

- ENABLE_FIREWALL
	- 防火墙功能，应对高安芯片内容保护的需求需要开启该配置
- MEMORY_PROTECT_TYPE
	- 内容保护 protect buffer flag，指示哪些类型的 buffer 开启了保护
	- 如果芯片烧写了内容保护 OTP 模式，则 loader 会自动探测生成该属性值，否则将采用用户设置的属性值
	- 如果用户要自行设置该属性，要求置上 GXFW_BUFFER_SOFT (1 << 15) 表示该配置为用户自定义的软件模式
- ENABLE_SECURE_ALIGN
	- 强制对齐访问功能，应对高安芯片 DDR 扰乱的需求需要开启该配置
