#include "gui_core.h"
#include "module/pm/gxpm_urlcontrol.h"

#define GX_NET_PM_REGISTER(name)   do {                 \
	extern NetPmCtrl netpmctrl_##name;                      \
	extern void GxNetUrlModuleRegister(NetPmCtrl *cls); \
	GxNetUrlModuleRegister(&netpmctrl_##name);            \
} while(0)

void GxNetPm_ModuleRegister(void)
{
    GX_NET_PM_REGISTER(server);
}

