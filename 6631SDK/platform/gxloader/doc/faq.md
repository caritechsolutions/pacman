# FAQ

### NFS 挂载

**Question**:
更新了GXLOADER之后NFS挂载不上了？

**Answer**:
如果能ping通但挂载不上，确认下nfs相关路径是否正确；如果ping不通，确认下.config中的BOARD选择是否正确，如果正确确认相应BOARD下的网络相关复用配置是否正确。


### 不能启动

**Question**:
更新了GXLOADER之后串口只打印了XRUN之后就没信息了？

**Answer**:
小封装的芯片如dvbc-3201、3231、dvbs-6601等，使用了一个gpio进行了ddr的复位控制。确认board-init.c中的相应配置是否正确。


**Question**:
更新了GXLOADER之后打印了XRUN以及"the memsize parsed from cmdline is"就没信息了。

**Answer**:
确认下board-init.c中是不是修改了用于ddr复位控制的gpio的复用。


### 不能下载
**Question**:
使用网络下载时，可以ping通板子，但是一直显示超时

**Answer**:
检查防火墙是否关闭

### 工程调试

**Question**:
怎么调试工程？


**Answer**:
GDB 调试 GxLoader 使用 JTAG 或者 Jlink 工具，C-SKY 芯片使用 JTAG，ARM 芯片使用 Jlink。

除了工具的使用，调试之前还要确定下对应板级的管脚复用，路径在 “gxloader/board/<chip>/board-<type>/board-init.c”，因为要 GDB 调试， 管脚复用中的 DBG 系列的管脚的复用功能统一选择 0。如下代码所示：

```
{_RIGHT_, 1, 33, MP_INV_V,MP_INV_V,  0}, //NC |DBGTDI/PORT01(PMUPORT01)
{_RIGHT_, 2, 34, 123,     MP_INV_V,  0}, //NC |DBGDTO/PORT02(PMUPORT02)/CEC
{_RIGHT_, 3, 35, 96,      MP_INV_V,  0}, //NC |DBGTMS/PORT03(PMUPORT03)/NFRDY0
{_RIGHT_, 4, 36, 96,      MP_INV_V,  0}, //NC |DBGTCK/PORT04(PMUPORT04)/NFOE
{_RIGHT_, 5, 37, 96,      MP_INV_V,  0}, //NC |DBGTRST/PORT05(PMUPORT05)/NFCLE
```

GDB 调试 GxLoader 有两种方式：

第一种方式只能调试 GxLoader 的 stage2 代码，并且 stage2 代码会被执行两次，这种方式也用于调试内核和用用代码；第二种方式可以调试 GxLoader 的 stage1 和 stage2 代码。

第一种调试方式:

1. 编译 debug 版本，注意：debug 版本并不是说关了编译优化，debug 相比 release 只是在跳转内核之前增加了３秒倒计时，可以在倒计时到之前按 enter 键进入命令行，使程序停在 Loader 中，方便 GDB 连接

   ```
   ./build <chip> <board> debug
   ```

2. 生成第一个 loader 程序，把编译好的程序 loader-sflash.bin下载到板子

3. 更改刚才编译生成的 .config 文件中的 ENABLE_GDB_DEBUG

   ```
   ENABLE_GDB_DEBUG = y
   ```

4. 执行 make clean;make，生成第二个 loader 程序，由于打开了 ENABLE_GDB_DEBUG，编译脚本会自动把 Os 优化改成 O0 的。

5. 启动板子，显示倒计时的时候，按 enter 键使程序停在 Loader 的命令行

6. 连接 jtag 或者 jlink

7. 执行 csky-elf-gdb loader.elf 或者 arm-none-eabi-gdb loader.elf 调试生成的第二个程序

**调试分析**

根据上面步骤，总共编译了两次，生成了两个 loader 程序，第一个程序是要下载到板子的，包含 stage1 和 stage2 代码，这个程序的目的就是能够使板子能正常运行起来，以便 GDB 调试。最终我们要调试的是更改了 "ENABLE_GDB_DEBUG = y" 后重新生成的第二个 loader 程序，第二个 loader 程序相比第一个程序的区别是，不会再去配置 PLL 和 DDR，并且关闭了编译优化，且程序的运行地址都是在 DDR 内存 。

**调试注意点**

1. 在第 5 步的时候，板子其实已经执行一遍 stage2 了，然后 GDB 调试第二个程序，也要执行一遍 stage2，很多一些硬件寄存器会被配两遍，某些硬件模块已经在工作了，在重新配置的工作中会导致硬件模块工作不正常，甚至直接导致板子死机。目前只发现 gx_show_logo 函数执行两次会死机。

第二种调试方式:

 1. 更改 gxloader/makefile 文件，把 else 下面的 CFLAGS 改成 CFLAGS  = -g -O0

    ```
    ifeq ($(ENABLE_GDB_DEBUG), y)
    CFLAGS  = -g -O0 
    else
    #CFLAGS  = -Os -g -ffunction-sections
    CFLAGS  = -g -O0 
    endif
    ```

2. 在要调试的地方前面加入下面两句代码，类似断点的作用

   ```
   volatile unsigned int while_flag = 1;
   while(while_flag);
   ```

3. 执行编译，release 版本和 debug 版本都可以

   ```
   ./build <chip> <board> release
   ```

4. 把编译生成的程序下载到板子

5. 更改 .gdbinit 文件，去掉文件中的 load

6. 开机启动，程序应该会停在 第 2 步加入的 while 循环

7. 执行 csky-elf-gdb loader.elf 或者 arm-none-eabi-gdb loader.elf 调试

8. GDB 调试把 第 2 步的 while_flag 置 0，这样就可以跳过 while 循环，继续调试下去

**调试分析**

这种方式是直接调试下载到板子里面的程序，为了能停在要调试的地方之前，加了 while(while_flag)，并且要去掉 .gdbinit 中的 load。这种调试方式方便用于调试 stage1 和 某些不能执行两遍的 stage2代码（如 show_logo）

**调试注意点**

1. 要注意第 5 步，.gdbinit 去掉 load
2. 第 2 步的 volatile 在关闭优化后加不加不影响，但是如果在没更改 Makefile 的优化选项时，必须要加 volatile
3. 由于 Makefile 中把优化选项关掉了，stage1 代码有可能过大，导致编译不过，这种情况只有在开启安全启动时会出现。


- [分区表](./partition.md)：gxloader启动时，若未找到partition分区，则使用board-init.c中的分区配置，该配置可由用户自行修改。发布的board-init.c中默认分区为空。
    -   partition的table表需存放在flash的0-1M区域内的512B对齐处，gxloader会以512B为步进在0-1M区域内查找table表的magic。


