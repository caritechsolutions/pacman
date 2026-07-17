#include "user.h"

#if (defined CONFIG_ARCH_CKMMU_SCORPIO || defined CONFIG_ARCH_CKMMU_CYGNUS) && defined CONFIG_ENABLE_FIREWALL
void scorpio_user_firewall_init(void)
{
	int size = 0;
	int protect_flag = 0;

	protect_flag = protect_flag_probe();
	if (protect_flag == 0)
		protect_flag = CONFIG_MEMORY_PROTECT_TYPE;

#if defined CMDLINE_ESAMEM_START && defined CMDLINE_ESVMEM_START
	if (protect_flag & GXMEM_FLAG_DEMUX_ES) {
		size = CMDLINE_ESAMEM_SIZE;
		gx_mem_get_int32_value("esamem_sub", &size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_ES, 1, CMDLINE_ESAMEM_START, size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_ES, 3, CMDLINE_ESAMEM_START, size);

		size = CMDLINE_ESVMEM_SIZE;
		gx_mem_get_int32_value("esvmem_sub", &size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_ES, 0, CMDLINE_ESVMEM_START, size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_ES, 2, CMDLINE_ESVMEM_START, size);
	}
#endif
#if defined CMDLINE_TSRMEM_START
	if (protect_flag & GXMEM_FLAG_DEMUX_TSR) {
		size = CMDLINE_TSRMEM_SIZE;
		gx_mem_get_int32_value("tsrmem_sub", &size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_TSR, 0, CMDLINE_TSRMEM_START, size);
	}
#endif
#if defined CMDLINE_TSWMEM_START
	if (protect_flag & GXMEM_FLAG_DEMUX_TSW) {
		size = CMDLINE_TSWMEM_SIZE;
		gx_mem_get_int32_value("tswmem_sub", &size);
		demux_SMP_config(GXMEM_FLAG_DEMUX_TSW, 0, CMDLINE_TSWMEM_START, size);
	}
#endif
#ifdef CMDLINE_AFWMEM_START
	if (protect_flag & GXMEM_FLAG_AUDIO_FIRMWARE) {
		size = CMDLINE_AFWMEM_SIZE;
		gx_mem_get_int32_value("afwemem_sub", &size);
		audio_SMP_config(GXMEM_FLAG_AUDIO_FIRMWARE, 0, CMDLINE_AFWMEM_START, size);
	}
#endif
#ifdef CMDLINE_VFWMEM_START
	if (protect_flag & GXMEM_FLAG_VIDEO_FIRMWARE) {
		size = CMDLINE_VFWMEM_SIZE;
		gx_mem_get_int32_value("vfwmem_sub", &size);
		video_SMP_config(GXMEM_FLAG_VIDEO_FIRMWARE, 0, CMDLINE_VFWMEM_START, size);
	}
#endif
#ifdef CMDLINE_SCPUMEM_START
	if (protect_flag & GXMEM_FLAG_SCPU) {
		size = CMDLINE_SCPUMEM_SIZE;
		gx_mem_get_int32_value("scpumem_sub", &size);
		scpu_SMP_config(GXMEM_FLAG_SCPU, 0, CMDLINE_SCPUMEM_START, size);
	}
#endif

}
#endif
