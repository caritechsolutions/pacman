PMU工程说明
目前PMU工程的MCU是51单片机,有256字节的ram,8KB的程序空间,主频为27MHZ
一.编译环境安装
安装SDCC编译器 3.6.0 版本
1.从 http://yun.nationalchip.com:10000/v/link/view/cbeee2c1551144c794c603fddde889ab 下载
  sdcc-3.6.0-i386-unknown-linux2.5.tar.bz2 文件
 (该链接打不开的话可以从 SDCC 的服务器 https://sourceforge.net/projects/sdcc/files/sdcc-linux-x86/3.6.0/ 下载)
2.sdcc-3.6.0-i386-unknown-linux2.5.tar.bz2,解压后把里面的bin目录加入到PATH(系统环境变量)里面

二.编译
cd gxlowpower/xx51
编译命令: ./build gx51
清除命令: ./build clean
编译出来的文件为gxlowpower.fw

三.运行
1.拷贝编译好的gxlowpower.fw文件到/lib/firmware目录下
2.具体使用方法看include/common/gxpm.h

四.工程目录文件介绍
arch/ mcu硬件相关文件
build build文件,用于编译
driver/ 驱动,包括红外,GPIO,CEC,功耗管理
include/ 需要被应用层调用的头文件
makefile/ 用于编译
main.c   主函数包含文件

五.二次开发
1.目前默认固件功能:PMU固件运行,使CPU进入低功耗状态,之后一直等待红外遥控信号和电
  视机唤醒信号唤醒CPU和电视机
2.支持的驱动外设包括红外.GPIO,CEC,功耗管理.在include文件夹中的头文件包含对应的
  API函数.红外目前仅支持NEC和松下40bit协议.
3.可以查看main.mem文件知悉ram空间占用大小和程序空间占用大小
