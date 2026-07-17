#include "user.h"

// 修改说明参考 http://git.nationalchip.com/redmine/issues/362550

#if defined CONFIG_ARCH_ARM_VEGA && defined CONFIG_ENABLE_FIREWALL
void vega_user_firewall_init(void)
{
	char *pExt = NULL;
	int size = 0, rd = 0, wr = 0, compatibility = 0;

#if defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)
	firewall_config_filter(CMDLINE_SVPUMEM_START, CMDLINE_SVPUMEM_SIZE, 0x00002000, 0x00002000, FILTER_FLAG_FORCE_ALIGNMENT);
#endif

	// 该接口只支持内容保护软件模式，OTP 模式直接退出
	if ((CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_SOFT) == 0)
		return;

	if ((pExt = strrchr(CHIP_BOARD, '-')) != NULL && memcmp(pExt+1, "NNM", 3) == 0)
		compatibility = 1;

	// 在 NNR 方案正式启用前都采用兼容模式
	compatibility = 1;

#if defined CMDLINE_ESAMEM_START && defined CMDLINE_ESVMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_ES) {
		if (compatibility == 1) { // 兼容模式
			rd = 0x00920900;
			wr = 0x00358012;
		} else { // Nagra 模式 (仅支持 PB / TP 权限输出)
			rd = 0x00920900;
			wr = 0x00340002;
		}

		size = CMDLINE_ESAMEM_SIZE;
		gx_mem_get_int32_value("esamem_sub", &size);
		firewall_config_filter( CMDLINE_ESAMEM_START, size, rd, wr, FILTER_FLAG_FORCE_ALIGNMENT);

		size = CMDLINE_ESVMEM_SIZE;
		gx_mem_get_int32_value("esvmem_sub", &size);
		firewall_config_filter( CMDLINE_ESVMEM_START, size, rd, wr, FILTER_FLAG_FORCE_ALIGNMENT);
	}
#endif
#if defined CMDLINE_TSRMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_TSR) {
		if (compatibility == 1) { // 兼容模式
			rd = 0x00900818;
			wr = 0x00358000;
		} else { // Nagra 模式
			rd = 0x00920808;
			wr = 0x00340004;
		}

		size = CMDLINE_TSRMEM_SIZE;
		gx_mem_get_int32_value("tsrmem_sub", &size);
		firewall_config_filter( CMDLINE_TSRMEM_START, size, rd, wr, FILTER_FLAG_FORCE_ALIGNMENT);
	}
#endif
#if defined CMDLINE_TSWMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_TSW) {
		if (compatibility == 1) { // 兼容模式
			rd = 0x00938800;
			wr = 0x00300014;
		} else { // Nagra 模式
			rd = 0x00920808;
			wr = 0x00340004;
		}

		size = CMDLINE_TSWMEM_SIZE;
		gx_mem_get_int32_value("tswmem_sub", &size);
		firewall_config_filter( CMDLINE_TSWMEM_START, size, rd, wr, FILTER_FLAG_FORCE_ALIGNMENT);
	#if defined CMDLINE_TSAMEM_START
		firewall_config_filter( CMDLINE_TSAMEM_START, CMDLINE_TSAMEM_SIZE, rd, wr, 0);
	#endif
	}
#endif

#if defined CMDLINE_AFBMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_AUDIO_FRAME)
		firewall_config_filter( CMDLINE_AFBMEM_START, CMDLINE_AFBMEM_SIZE, 0x00004800, 0x00000800, 0);
#endif

#if defined CMDLINE_VFBMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_VIDEO_FRAME)
		firewall_config_filter( CMDLINE_VFBMEM_START, CMDLINE_VFBMEM_SIZE, 0x00003100, 0x00003100, 0);
#endif

#if defined CMDLINE_TEEMEM_START && defined CONFIG_ENABLE_TEE
	if (compatibility == 1) { // 兼容模式
		rd = 0x80738020;
		wr = 0x80558020;
	} else { // Nagra 模式
		rd = 0x80700020;
		wr = 0x80500020;
	}

	size = CMDLINE_TEEMEM_SIZE;
	gx_mem_get_int32_value("teemem_sub", &size);
	firewall_config_filter( CMDLINE_TEEMEM_START, size, rd, wr, FILTER_FLAG_SECURE_CHECK | FILTER_FLAG_FORCE_ALIGNMENT);
#endif

#if defined CMDLINE_HDCPMEM_START
	if (compatibility == 1) { // 兼容模式
		rd = 0xC0000000;
		wr = 0xC0000000;
	} else { // Nagra 模式
		rd = 0xC0000000;
		wr = 0xC0000000;
	}

	size = CMDLINE_HDCPMEM_SIZE;
	firewall_config_filter( CMDLINE_HDCPMEM_START, size, rd, wr, FILTER_FLAG_SECURE_CHECK | FILTER_FLAG_FORCE_ALIGNMENT);
#endif
}
#endif
