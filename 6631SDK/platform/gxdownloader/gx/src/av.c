#include "gxdlp_api.h"
#include "common.h"
#include "config.h"

#include "gxav.h"
#include "av/gxav_common.h"
#include "av/hal/gxav_hal_vout.h"
#include "av/hal/gxav_hal_fb.h"
#ifdef NO_OS
#include "gxloader.h"
#include "av/gxavdev_init.hxx"
#include "av/chip_init.hxx"
#endif
#include "av.h"
#include "devapi/chip_info.h"

#ifdef NO_OS
#define _REGISTER_AV(chip)                                             \
	do {                                                          \
		gxav_chip_register_##chip();\
		gxav_init(NULL); \
	} while (0);

#define REGISTER_AV(chip) _REGISTER_AV(chip)
#else
#define REGISTER_AV(chip)
#endif

unsigned int avinit(void)
{
	unsigned int chip_definition;
	GxVideoOutProperty_Mode mformat;
	GxVoutResolution vres;
	GxVoutInitParams param = {.channels = GXAV_VOUT_HDMI|GXAV_VOUT_RCA};
	GxVoutChannelAutoFormat format = {.enable = 1, .pal = GXAV_VOUT_PAL, .ntsc = GXAV_VOUT_NTSC_M};
	int ret = 0;

#ifdef CONFIG_SCORPIO_TAURUS
	if (GXCHIP_IS_SCORPIO) {
		REGISTER_AV(scorpio);
	} else {
		REGISTER_AV(taurus);
	}
#else
	REGISTER_AV(CHIP);
#endif

	chip_definition = HD_DEFINITION;
	if (HD_DEFINITION == chip_definition)
	{
		vres.xres = 1024;
		vres.yres = 576;
	}
	else
	{
		vres.xres = 720;
		vres.yres = 576;
	}

        if (GxDLP_Get_TvStandard(&mformat))
                mformat = GXAV_VOUT_1080I_50HZ; //if get format from OEM.ini failed, set default format

        ret |= GxVoutHAL_Init(param);
        ret |= GxVoutHAL_ChannelSetAutoFormat(GXAV_VOUT_RCA, format);
        ret |= GxVoutHAL_ChannelSetVirtualResolution(GXAV_VOUT_HDMI, &vres);
        ret |= GxVoutHAL_ChannelSetFormat(GXAV_VOUT_HDMI, mformat);

        if (ret != 0)
		gxloge("av init failed\n");

	return chip_definition;
}
