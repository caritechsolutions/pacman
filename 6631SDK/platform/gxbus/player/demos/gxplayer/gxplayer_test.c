#include "xshell.h"
#include "log.h"

static int _gxplayer_help(int argc, char *argv[])
{
	unsigned int i = 0;
	ShellTable *table = Shell_GetCmdTable();

	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: [cmd]\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	for(i = 0; i < table->now; i++) {
		ShellCmd *cmd = table->cmd[i];
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "\t%s\n", cmd->brief);
	}

	return 0;
}

ShellCmd gGxPlayerHelp = {
	.name  = "help",
	.func  = _gxplayer_help,
	.brief = "help: gxplayer help command",
};

//#define ECOS_OS_CPULOAD
#ifdef ECOS_OS_CPULOAD

#include "gxos/gxcore_os_core.h"
#include <cyg/kernel/kapi.h>
#include <cyg/cpuload/cpuload.h>

cyg_uint32    calibration;
cyg_handle_t  ecos_cpuload_handle;
cyg_cpuload_t ecos_cpuload_info;

static void _cpuload_thread(void* arg)
{
	while(1) {
		cyg_uint32 average_point1s;
		cyg_uint32 average_1s;
		cyg_uint32 average_10s;

		cyg_cpuload_get(ecos_cpuload_handle, &average_point1s, &average_1s, &average_10s);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "[CPULOAD %d | 100ms: %3d%% | 1s: %3d%% | 10s: %3d%% ]\n", calibration,
				average_point1s, average_1s, average_10s);
		GxCore_ThreadDelay(1000);
	}
}
#endif

static void _cpuload_init(void)
{
#ifdef ECOS_OS_CPULOAD
	static int cpuload_thread_handle = 0;

	//calibration = 6140000;
	cyg_cpuload_calibrate(&calibration);
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "CPULOAD calibrate result: %u\n", calibration);

	cyg_cpuload_create(&ecos_cpuload_info, calibration, &ecos_cpuload_handle);

	GxCore_ThreadCreate("stack", &cpuload_thread_handle, _cpuload_thread, NULL, 100 * 1024,
			GXOS_DEFAULT_PRIORITY - 1);

	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Creating CPULOAD thread result: %u\n", calibration);
#endif
}

int GxCore_Startup(int argc, char **argv)
{
	extern ShellCmd gGxPlayerPlay;
	extern ShellCmd gGxPlayerStop;
	extern ShellCmd gGxPlayerPause;
	extern ShellCmd gGxPlayerResume;
	extern ShellCmd gGxPlayerInfo;
	extern ShellCmd gNetworkTest;

	Log_SetModule(LOG_GXPLAYER);

	Log_SetLevel (LOG_INFO|LOG_ERROR|LOG_WARNING);

	_cpuload_init();

	Shell_Init();

	Shell_AddCmd(&gGxPlayerInfo);

	Shell_AddCmd(&gGxPlayerPlay);

	Shell_AddCmd(&gGxPlayerStop);

	Shell_AddCmd(&gGxPlayerPause);

	Shell_AddCmd(&gGxPlayerResume);

	Shell_AddCmd(&gGxPlayerHelp);

	Shell_AddCmd(&gNetworkTest);

	Shell_Start();

	Shell_Loop();

	Shell_Stop();

	return 0;
}
