// create in 20141117
// v1.0

1，硬件信道自动测试程序，使用串口通讯方式，配合PC软件进行交互使用。

2，使用环境说明：
   1>，软件是基于GOXCEED软件平台开发，模块中使用的相关驱动接口是基于GOXCEED平台
   的，该模块软件是基于gx3201的goxceed平台调试验证。
   
   2>，ECOS下使用的普通的uart口，实现了控制交互。
   
   3>，LINUX下使用USB转串口的方式，实现控制交互。【STB部分使用USB转串口线和PC通
   讯。所以需要注意，USB转串口的驱动支持，具体设备支持，参考linux内核开启的驱动
   支持，目前最新公版的buildroot，默认支持：CP2101，Prolific 2303，FTDI。可以根
   据需求配置添加】
   【说明】，目前针对LINUX的方案，没有做USB设备的识别处理，也就是说如果USB口除了
   插了USB串口线，还插着U盘，那这部分功能也会存在问题，所以目前的软件，要求，在
   使用auto test功能的时候，USB口只能插USB转串口线。

3，交互命令。
   交互命令支持，见软件"autotest/common_commands.c"，使用参考，见文件
   "cmd_sample.txt"。
   【说明】，关于交互命令，其中PC端下行的命令，结尾需要添加换行符"\r\n"。

4，软件说明。
   1>，应用软件中添加了模块开关"Hardware Auto Test Support"，通过"make
   menuconfig"菜单配置，生成配置宏"AUTO_TEST_ENABLE"。

   2>，模块软件部分，分成几个部分：common， module，service以及demod相关。
       common: 需要注意的是common_commands.c，里面是对目前支持的控制命令的支持实
       现，如果没有新的控制命令的需求，可以保持目前的结构不变。
       module：基本上可以保持不变，只是提供了基本的功能实现，以及相关控制结构的
       定义。
       service：使用的目前goxceed平台提供的服务的框架定义。特别注意的是init的函
       数，common部分支持了很多的命令实现，但是真正被串口软件识别的命令必须在
       init函数中注册。同样不同信道的支持，也许要通过注册的方式添加软件识别。
       demod相关：目前只是实现了dvbs/s2的测试控制"autotest_dvbs.c"。后续，如果需
       要支持其他的信道，那只需要参考"autotest_dvbs.c"，实现对应的
       "autotest_xxx.c"，并且在"service_autotest.c"中初始化函数中注册关联。




