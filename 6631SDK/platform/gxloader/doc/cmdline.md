# Cmdline

cmdline 是用来向内核传递启动参数的，它可以指导内核如何启动和挂载根文件系统。cmdline 的格式是由多个项目用空格隔开，例如：

```
CMDLINE_VALUE = "mem=55M videomem=54168K esamem=128K esvmem=3796K tsrmem=512K tswmem=3688K vfwmem=364K afwmem=2M svpumem=832K
teemem=9M mem_end teemem_sub=7M tswmem_sub=3663744 tsrmem_sub=523392 console=ttyS0,115200 init=/init
root=/dev/nfs rw nfsroot=192.168.101.200:/opt/nfs,v3 ip=192.168.101.3"
```

- MEM 配置
  - mem 字段    ： 用于 REE 代码段、data 段、堆栈 等等
  - teemem 字段 ： 用于 TEE 代码段、data 段、堆栈 等等，还包括一块 REE / TEE 共享内存
	  - teemem_sub 字段: 表示 teemem 内实际 TEE 使用的大小 (不包括共享内存)，区域为从 teemem 字段起始位置开始 teemem_sub 大小
  - videomem 字段 : 用于音视频帧存管理
  - esamem 字段 : 高安方案下使用，用于安全播放的 audio ES 数据存储
  - esvmem 字段 : 高安方案下使用，用于安全播放的 video ES 数据存储
  - afwmem 字段 : 高安方案下使用，用于安全播放的 audio 固件数据存储
  - vfwmem 字段 : 高安方案下使用，用于安全播放的 video 固件数据存储
  - svpumem 字段 : 高安方案下使用，用于安全播放的 VPU 显示
  - tswmem 字段 : 主要是高安方案下使用，用于安全录制的 TS 数据存储
	  - tswmem_sub 字段: 类似 NNM 方案下使用，表示 tswmem 内实际可用的大小，区域为从 tswmem 字段起始位置开始 tswmem_sub 大小
  - tsrmem 字段 : 主要是高安方案下使用，用于安全回放的 TS 数据存储
	  - tsrmem_sub 字段: 类似 NNM 方案下使用，表示 tsrmem 内实际可用的大小，区域为从 tsrmem 字段起始位置开始 tsrmem_sub 大小
  - tsamem 字段 : 高安方案下有 AD 节目播放需求使用

- 通用配置

```
console=ttyS0,115200 init=/init
```

- NFS 配置

```
root=/dev/nfs rw nfsroot=192.168.101.200:/opt/nfs,v3 ip=192.168.101.3"
```
