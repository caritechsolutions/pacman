# 系统概述

GxLoader 是杭州国芯微自行开发、用于机顶盒芯片上的 bootloader 程序，具有良好的代码可扩展性、可维护性和可适用性。
支持 ARM 和 C-SKY 架构，包括：C-SKY 架构的GX3201、GX3113c、GX3211、GX6605s、Taurus、Gemini、Cygnus 、scorpio 系列芯片；ARM 架构的Sirius、Canopus、Vega 系列芯片。
支持引导 Linux、eCos、TEEOS、Nos 等操作系统。


Gxloader 支持如下多种功能：

- [引导功能](./boot.md)，支持多种芯片板级引导 Linux、eCos、TeeOS、Nos 等各种操作系统镜像。
- [安全启动](./secure_boot.md)，支持多种芯片板级以及对应的镜像签名加密工具。
- [固件功能](./bootfw.md)，支持编译生成.boot 固件，支持镜像烧写工具的固件。
- [平台库功能](./nos.md)，支持编译生成 nos 库以便二次开发，支持 OTA、BBT、SLT 等开发。
- [命令功能](./develop.md)，支持命令系统，方便用户扩展各类命令。


