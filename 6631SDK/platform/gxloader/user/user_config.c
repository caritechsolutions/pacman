#include "user_config.h"
#include "user.h"

static void user_meminfo_init(void)
{
	// 添加方案自定义 cmdline 字段
	// 例如 cmdline_add("teemem_sub=7M"); 表示 teemem 只保护 7M
}

static void user_firewall_init(void)
{
#ifdef CONFIG_ENABLE_FIREWALL
	// Firewall 配置
	//
	// int firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag);
	// int sram_firewall_config_filter(unsigned int addr, int size, int readable, int writable);

#if defined(CONFIG_ARCH_ARM_CANOPUS)
	canopus_user_firewall_init();
#endif

#if defined(CONFIG_ARCH_ARM_VEGA)
	vega_user_firewall_init();
#endif

#if defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_CYGNUS)
	scorpio_user_firewall_init();
#endif

#if defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	if (gxcore_chip_probe() == 0x3113)
		scorpio_user_firewall_init();
#endif

#endif
}

static void user_permit_init(void)
{
#ifdef CONFIG_ENABLE_TEE
	// TEE permit 配置
	//
	// void permit_set_config(unsigned int module, unsigned int flags);
	//
#if defined(CONFIG_ARCH_ARM_VEGA)
	permit_set_config(GXSE_PERMIT_MOD_VPU_SEC,  PERMIT_FLAG_REE_UNREADABLE|PERMIT_FLAG_REE_UNWRITABLE);
	permit_set_config(GXSE_PERMIT_MOD_HDMI_SEC, PERMIT_FLAG_REE_UNREADABLE|PERMIT_FLAG_REE_UNWRITABLE);
#endif

#ifdef CONFIG_MEMORY_PROTECT_TYPE
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_AUDIO_FRAME)
		permit_set_config(GXSE_PERMIT_MOD_ADSP_SEC, PERMIT_FLAG_REE_UNWRITABLE);
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_VIDEO_FRAME)
		permit_set_config(GXSE_PERMIT_MOD_VDEC_SEC, PERMIT_FLAG_REE_UNWRITABLE);
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_TSW)
		permit_set_config(GXSE_PERMIT_MOD_DEMUX_TS, PERMIT_FLAG_REE_UNWRITABLE);
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_ES)
		permit_set_config(GXSE_PERMIT_MOD_DEMUX_ES, PERMIT_FLAG_REE_UNWRITABLE);
#endif

	permit_set_config(GXSE_PERMIT_MOD_APLAY_SEC   , PERMIT_FLAG_REE_UNWRITABLE);
	permit_set_config(GXSE_PERMIT_MOD_DEMUX_KEY   , PERMIT_FLAG_REE_UNWRITABLE);
	permit_set_config(GXSE_PERMIT_MOD_SYSCLK      , PERMIT_FLAG_REE_UNWRITABLE);
	permit_set_config(GXSE_PERMIT_MOD_CRYPTO      , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_MSR2_KLM    , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_AKL         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_M2M         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_RNG         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_TRNG        , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_RNG_CERT    , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_HASH        , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_RCC         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_OTPC        , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_A7_DAP      , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SDC         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_PKA         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_RSA         , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_FIREWALL    , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_AHB_FIREWALL, PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SENSOR      , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SENSOR0_27M , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SENSOR1_27M , PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SENSOR0_198M, PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
	permit_set_config(GXSE_PERMIT_MOD_SENSOR1_198M, PERMIT_FLAG_REE_UNWRITABLE | PERMIT_FLAG_REE_UNREADABLE);
#endif
}

void user_init(void)
{
	user_meminfo_init();
	user_firewall_init();
	user_permit_init();
}

