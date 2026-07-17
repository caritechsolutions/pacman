1.编译方式：
　./build即可看到帮助

2.固件
  当前目录下的gxlowpower.fw

3.使用方式
  <1>固件使用方式
	将生成的gxlowpower.fw拷贝到根文件系统的/lib/firmware/目录下
  <2>进入lowpower方式
	调用GxCore_Halt进入低功耗，关于GxCore_Halt的使用参见API说明

4.通过一个gpio用于关闭在低功耗状态不使用的电路
  使用方法和支持红外多键值进入休眠的调用方法一样，配置char * cmdline;参数"powercut=virgpio,1/0". virgpio为需要设置的虚拟gpio号，1/0为对虚拟gpio进行拉高/拉低.
同时支持红外多键值进入休眠和通过一个gpio用于关闭在低功耗状态不使用的电路的配置如下：
char cmdline[]="keys=0x7F8007F8,0x7F80E718,0x7F80A758 powercut=201,0";

5.对于NEC协议的遥控器支持传入双字节的用户码与系统码来唤醒
  对于NEC协议的遥控器支持了传入16bit的用户码与系统码来唤醒和32bit的全键值唤醒。
     例如：
	char cmdline[]="keys=0x7F8007F8,0x7F80E718,0x7F80A758 powercut=201,0";
     也可以写成如下形式:
	char cmdline[]="keys=0x7F07,0x7FE7,0x7F80A758 powercut=201,0";

6.flash接口使用说明
  a) 增加三个flash接口函数，包含gxcomm.h头文件即可直接调用，不需要配置。
     int flash_read(unsigned int addr, unsigned char *buf,unsigned int len);
     int flash_write(unsigned int addr,unsigned char *buf,unsigned int len);
     void flash_erase(unsigned int addr);
  b) flash_erase以block为单位，即一次擦除64K。
  c) 要确保低功耗下用于关闭外围电路的gpio没有将spi nor flash的供电关闭。
  d) 全部读写擦均使用最标准的方式，不支持4 byte地址模式，所以如果应用使用的是32Mbytes FLASH的话，要将操作区域放在0～16Mbytes区间内。
