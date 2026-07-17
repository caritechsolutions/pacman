# 命令功能

cmd 支持

# USB 升级功能

USB Recovery是在启动后，gxloader 去检测升级的操作，相关配置在对应板级下的release.config，或者软件目录下的.config。根据不同的编译方法选择。

下面介绍主要的配置：

- ENABLE_USB = y (必选 y，USB使能)
- ENABLE_IRR = y (必选 y，进入升级界面后,需要遥控器选择是否升级)
- ENABLE_FLASH_FULLFUNCTION = y (必选 y，Flash操作)
- ENABLE_GDI = y (必选 y，UI界面能出来)
- ENABLE_USB_RECOVER = y (必选 y，USB Recovery功能打开)
- CONFIG_RECOVERY_IMAGE_SIZE = 0x400000 (必选 升级文件大小，就是拷贝到U盘内准备升级的整个image的大小)
- CONFIG_RECOVERY_IMAGE = test.rev (可选 定义此宏，则升级文件名为test.rev，若不定义此宏,升级文件名为recovery.rcv，默认不定义)
- USB_RECOVER_WHOLE_IMAGE = n (可选 y 升级整个image包括bootloader，n 跳过bootloader升级。默认为n)
