# 系统概述

GxDownloader 是基于 GxAPI 开发的国芯微机顶盒镜像升级程序，支持 ARM 和 C-SKY 架构，包括：GX3201、GX3113c、GX3211、GX6605s、Sirius、Taurus、Gemini 系列芯片。主要提供了两种模式（有线模式、无线模式）升级镜像功能。有线模式支持 USB、UART、Flash；无线模式支持 OTA、NET。支持安全升级功能，支持多语言界面二次开发。它有两种（工程模式和方案模式）软件集成模式。工程模式支持 Nos、Ecos 两种系统，方案模式支持 Ecos、Linux 系统。具有很好的代码可扩展性、可维护性和可适用性。

名词解释表

|  名词    |  解释    |
| ---- | ---- |
| 分区   | GxPartition 描述 Flash 组织的数据结构  |
| DLP     | Downloader Partition 用于方案和 GxDownloader 交互信息分区   |
| OTA     | Over-The-Air 空中升级    |
