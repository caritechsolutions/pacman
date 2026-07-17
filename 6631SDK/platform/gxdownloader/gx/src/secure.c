#include "gxdlp_api.h"
#include "common.h"
#include "config.h"
#ifdef NO_OS
#include "gxsecure/gxsecure_dev_init.hxx"
#endif
#include "devapi/chip_info.h"

void secure_init(void)
{
#ifdef NO_OS
#ifdef CONFIG_SCORPIO_TAURUS
	if (GXCHIP_IS_SCORPIO) {
		REGISTER_GXSE_MODULE(scorpio, misc, otp);
		REGISTER_GXSE_MODULE(scorpio, misc, firewall);
	} else {
		REGISTER_GXSE_MODULE(taurus, misc, otp);
		REGISTER_GXSE_MODULE(taurus, misc, firewall);
	}
#else
	REGISTER_GXSE_MODULE(CHIP, misc, otp);
	REGISTER_GXSE_MODULE(CHIP, misc, firewall);
#endif
#endif
}
