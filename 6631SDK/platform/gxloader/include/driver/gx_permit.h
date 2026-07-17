#ifndef _GX_PERMIT_H_
#define _GX_PERMIT_H_

enum {
    GXSE_PERMIT_MOD_SRAM         = 0x1,    ///< SRAM 模块
    GXSE_PERMIT_MOD_SRAM_BOTTOM  = 0x10,   ///< SRAM 模块 bottom section
    GXSE_PERMIT_MOD_SRAM_TOP     = 0x11,   ///< SRAM 模块 top section
    GXSE_PERMIT_MOD_SRAM_CONFIG  = 0x12,   ///< SRAM 模块分界线配置

    GXSE_PERMIT_MOD_SDC          = 0x505,   ///< SDC 模块
    GXSE_PERMIT_MOD_PKA          = 0x506,   ///< PKA 模块
	GXSE_PERMIT_MOD_FIREWALL     = 0x50e,   ///< FIREWALL 模块
	GXSE_PERMIT_MOD_SENSOR       = 0x510,   ///< Sensor 模块

	GXSE_PERMIT_MOD_CRYPTO       = 0x600,   ///< ACPU Crypto 模块
	GXSE_PERMIT_MOD_MSR2_KLM     = 0x601,   ///< MSR2 KLM 模块
	GXSE_PERMIT_MOD_M2M          = 0x602,   ///< Secure M2M 模块
	GXSE_PERMIT_MOD_SYS_CONFIG   = 0x604,   ///< System Config 模块
    GXSE_PERMIT_MOD_TRNG         = 0x605,   ///< TRNG 模块
    GXSE_PERMIT_MOD_HASH         = 0x606,   ///< ACPU Hash 模块
    GXSE_PERMIT_MOD_RCC          = 0x608,   ///< RCC 模块
    GXSE_PERMIT_MOD_OTPC         = 0x609,   ///< OTP Controller 模块
    GXSE_PERMIT_MOD_A7_DAP       = 0x60a,   ///< Cotex-A7 DAP 模块
    GXSE_PERMIT_MOD_JTAG         = 0x610,   ///< System Config 模块中的 JTAG 控制寄存器
    GXSE_PERMIT_MOD_SVP          = 0x611,   ///< SVP 安全寄存器
    GXSE_PERMIT_MOD_RNG          = 0x612,   ///< System Config 模块中的 ACPU RNG 相关寄存器
    GXSE_PERMIT_MOD_SYSCLK       = 0x613,   ///< System Config 模块中的时钟寄存器
	GXSE_PERMIT_MOD_AKL          = 0x614,   ///< AKL 模块
	GXSE_PERMIT_MOD_CLOCK_CONFIG = 0x615,   ///< Clock Config 模块
	GXSE_PERMIT_MOD_PIN_CONFIG   = 0x616,   ///< PIN Config 模块
	GXSE_PERMIT_MOD_RNG_CERT     = 0x617,   ///< RNG CERT 模块

    GXSE_PERMIT_MOD_GP           = 0x700,   ///< GP 模块
    GXSE_PERMIT_MOD_DEMUX_TS     = 0x710,   ///< DEMUX 模块中的 TS buffer 寄存器
    GXSE_PERMIT_MOD_DEMUX_ES     = 0x711,   ///< DEMUX 模块中的 ES buffer 寄存器
    GXSE_PERMIT_MOD_DEMUX_KEY    = 0x712,   ///< DEMUX 模块中的解扰寄存器
    GXSE_PERMIT_MOD_ADSP_SEC     = 0x717,   ///< Audio DSP 模块中的安全寄存器
    GXSE_PERMIT_MOD_APLAY_SEC    = 0x719,   ///< Audio Play 模块中的安全寄存器
    GXSE_PERMIT_MOD_GP_SEC       = 0x71c,   ///< GP 模块中的安全寄存器
    GXSE_PERMIT_MOD_HDMI_SEC     = 0x71f,   ///< HDMI 模块中的安全寄存器
    GXSE_PERMIT_MOD_VPU_SEC      = 0x721,   ///< VPU 模块中的安全寄存器
    GXSE_PERMIT_MOD_VDEC_SEC     = 0x724,   ///< Video Decoder 模块中的安全寄存器
	GXSE_PERMIT_MOD_AHB_FIREWALL = 0x725,   ///< AHB Firewall 模块
	GXSE_PERMIT_MOD_RSA          = 0x726,   ///< RSA 模块
	GXSE_PERMIT_MOD_TSIO         = 0x727,   ///< TSIO 模块

	GXSE_PERMIT_MOD_SENSOR0_27M  = 0x800,   ///< Sensor0 27M 模块
	GXSE_PERMIT_MOD_SENSOR1_27M  = 0x801,   ///< Sensor1 27M 模块
	GXSE_PERMIT_MOD_SENSOR0_198M = 0x802,   ///< Sensor0 198M 模块
	GXSE_PERMIT_MOD_SENSOR1_198M = 0x803,   ///< Sensor1 198M 模块
};

#define PERMIT_FLAG_REE_UNREADABLE  (0x1<<0)
#define PERMIT_FLAG_REE_UNWRITABLE  (0x1<<1)

void permit_init(void);
void permit_set_config(unsigned int module, unsigned int flags);

#endif
