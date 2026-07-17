#ifndef __GX_FIREWALL_H__
#define __GX_FIREWALL_H__

#define MASTER_DEMUX_ES             (1<<1)
#define MASTER_DEMUX_TS_WRITE       (1<<2)
#define MASTER_DEMUX_TS_READ        (1<<3)
#define MASTER_DEMUX_OTHER          (1<<4)
#define MASTER_PIDFILTER            (1<<7)
#define MASTER_GSE                  (1<<8)
#define MASTER_GP_READ              (1<<9)
#define MASTER_GP_WRITE             (1<<10)
#define MASTER_VIDEO_FW             (1<<11)
#define MASTER_VIDEO_FRAME          (1<<12)
#define MASTER_AUDIODEC_FW          (1<<13)
#define MASTER_AUDIODEC_CM          (1<<14)
#define MASTER_PP                   (1<<15)
#define MASTER_GA                   (1<<17)
#define MASTER_JPEG                 (1<<18)
#define MASTER_VPU                  (1<<19)
#define MASTER_SPI                  (1<<20)
#define MASTER_AUDIOPLAY            (1<<21)
#define MASTER_M2M_NORMAL_ENCRYPT   (1<<22)
#define MASTER_M2M_NORMAL_DECRYPT   (1<<23)
#define MASTER_M2M_PRV_ENCRYPT      (1<<24)
#define MASTER_M2M_PRV_DECRYPT      (1<<25)
#define MASTER_M2M_FW_DECRYPT       (1<<26)
#define MASTER_RCC                  (1<<28)
#define MASTER_HASH                 (1<<29)
#define MASTER_MAC_USB_NFC          (1<<30)
#define MASTER_A7                   (1<<31)
#define MASTER_SCPU                 (1<<5) //only taurus/gemini

#define FILTER_FLAG_SECURE_CHECK    (1<<0)
#define FILTER_FLAG_HIDE_ERROR      (1<<1)
#define FILTER_FLAG_FORCE_ALIGNMENT (1<<3)

/*
 * Firewall 驱动初始化
 *
 * \return
 */
void firewall_init(void);

/*
 * Firewall DDR 权限配置接口
 * 1) 该接口必须在 firewall_init 后才能调用
 * 2) 如果要修改已存在区域的权限，则 addr 和 size 要确保和之前的区域完全一致，且本次调用会覆盖上一次配置的权限
 * 3) filter 的个数在不同芯片是不一样的，当个数不够的时候会返回配置失败
 *
 * \param addr 表示 DDR 区间的起始地址，注意该参数需要 1kB 对齐
 * \param size 表示 DDR 区间的长度范围，注意该参数需要 1kB 对齐
 * \param master_rd_permission 表示各个 Master 对该 DDR 区间的读权限，每 bit 的含义请参考上面的 MASTER_XXX 宏定义
 * \param master_wr_permission 表示各个 Master 对该 DDR 区间的写权限，每 bit 的含义请参考上面的 MASTER_XXX 宏定义
 * \param flag 表示特定的 flag 集合，bit0 表示是否能
 *		bit[0] = 1 : 开启 CPU 访问的 TEE/REE 隔离功能，即REE CPU 不可读写; bit[0] = 0 : 关闭 TEE/REE 隔离功能
 *
 * \return
 * \retval 0 配置成功
 * \retval <0 配置失败
 */
int firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission,int flag);
// TODO: DDR Firewall 支持单独配置 filter 的 CPU 权限，同 firewall_config_sram_filter_cpu
//int firewall_config_filter_cpu(unsigned int addr, int size, int readable, int writable, int flag);

/*
 * Firewall SRAM 权限配置接口
 * 1) 该接口最大支持配置两个区域
 * 2) 该接口必须先配置一块区域，该区域的起始 addr = SRAMBASE，否则会报错
 * 3) 配置SRAM 完整区域的权限，需要设置 addr = SRAMBASE，size = SRAM_SIZE
 *    ┌───────────────────┐
 *    │                   │
 *    │                   │
 *    │                   │
 *    └───────────────────┘
 * 4.1) 将 SRAM 划分成两个区域，然后配置区域A，需要设置 addr = SRAMBASE，size < SRAM_SIZE
 *    ┌───────────────────┐
 *    │           │       │
 *    │  area A   │       │
 *    │           │       │
 *    └───────────────────┘
 * 4.2) 将 SRAM 划分成两个区域，然后配置区域B，需要设置 addr = SRAMBASE + area A'size，size = SRAM_SIZE - area A'size
 *    ┌───────────────────┐
 *    │           │       │
 *    │           │area B │
 *    │           │       │
 *    └───────────────────┘
 *
 * \param addr 表示 SRAM 区间的起始地址
 * \param size 表示 SRAM 区间的长度范围
 * \param readable 表示该 SRAM 区间 CPU 是否可读
 * \param writable 表示该 SRAM 区间 CPU 是否可写
 * \param flag 表示特定的 flag 集合
 *		bit[0] = 1 : TEE; bit[0] = 0 : REE
 *
 * \return
 * \retval 0 配置成功
 * \retval <0 配置失败
 */
int firewall_config_sram_filter_cpu(unsigned int addr, int size, int readable, int writable, int flag);
#endif
