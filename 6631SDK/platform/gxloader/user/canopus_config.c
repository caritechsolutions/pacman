#include "user.h"

#if defined CONFIG_ARCH_ARM_CANOPUS && defined CONFIG_ENABLE_FIREWALL
void canopus_user_firewall_init(void)
{
	int sub_protect_size = 0;
	unsigned int buf_addr = 0, buf_size = 0;

#if defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)
	buf_addr = CMDLINE_SVPUMEM_START;
	buf_size = CMDLINE_SVPUMEM_SIZE;
	firewall_config_filter(buf_addr, buf_size, MASTER_VPU, MASTER_VPU, 0);
#endif
	// 该接口只支持内容保护软件模式，OTP 模式直接退出
	if ((CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_SOFT) == 0)
		return;

#if defined CMDLINE_ESAMEM_START && defined CMDLINE_ESVMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_ES) {
		if (gx_mem_get_int32_value("esamem_sub", &sub_protect_size) == 0)
			firewall_config_filter( CMDLINE_ESAMEM_START, sub_protect_size ,0x31004800, 0x06000002,FILTER_FLAG_FORCE_ALIGNMENT);
		else
			firewall_config_filter( CMDLINE_ESAMEM_START, CMDLINE_ESAMEM_SIZE ,0x31004800, 0x06000002,0);

		if (gx_mem_get_int32_value("esvmem_sub", &sub_protect_size) == 0)
			firewall_config_filter( CMDLINE_ESVMEM_START, sub_protect_size ,0x31004800, 0x06000002,FILTER_FLAG_FORCE_ALIGNMENT);
		else
			firewall_config_filter( CMDLINE_ESVMEM_START, CMDLINE_ESVMEM_SIZE ,0x31004800, 0x06000002,0);
	}
#endif
#if defined CMDLINE_TSRMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_TSR) {
		if (gx_mem_get_int32_value("tsrmem_sub", &sub_protect_size) == 0)
			firewall_config_filter( CMDLINE_TSRMEM_START, sub_protect_size ,0x31000808, 0x06800004,FILTER_FLAG_FORCE_ALIGNMENT);
		else
			firewall_config_filter( CMDLINE_TSRMEM_START, CMDLINE_TSRMEM_SIZE ,0x31000808, 0x06800004,0);
	}
#endif
#if defined CMDLINE_TSWMEM_START
	if (CONFIG_MEMORY_PROTECT_TYPE & GXMEM_FLAG_DEMUX_TSW) {
		if (gx_mem_get_int32_value("tswmem_sub", &sub_protect_size) == 0)
			firewall_config_filter( CMDLINE_TSWMEM_START, sub_protect_size, 0x31400808, 0x06000004,FILTER_FLAG_FORCE_ALIGNMENT);
		else
			firewall_config_filter( CMDLINE_TSWMEM_START, CMDLINE_TSWMEM_SIZE, 0x31400808, 0x06000004,0);
	}
#endif
#if defined CMDLINE_TEEMEM_START && defined CONFIG_ENABLE_TEE
	if (gx_mem_get_int32_value("teemem_sub", &sub_protect_size) == 0)
		firewall_config_filter( CMDLINE_TEEMEM_START, sub_protect_size,0x8fd00000, 0x8fd00000,FILTER_FLAG_SECURE_CHECK | FILTER_FLAG_FORCE_ALIGNMENT);
	else
		firewall_config_filter( CMDLINE_TEEMEM_START, CMDLINE_TEEMEM_SIZE,0x8fd00000, 0x8fd00000,FILTER_FLAG_SECURE_CHECK);
#endif
}
#endif
