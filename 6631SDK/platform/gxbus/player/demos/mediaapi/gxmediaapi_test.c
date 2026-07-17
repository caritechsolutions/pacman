#include "xshell.h"
#include "log.h"

static int _gxmediaapi_help(int argc, char *argv[])
{
	unsigned int i = 0;
	ShellTable *table = Shell_GetCmdTable();

	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: [cmd]\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	for(i = 0; i < table->now; i++) {
		ShellCmd *cmd = table->cmd[i];
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\t%s\n", cmd->brief);
	}

	return 0;
}

ShellCmd gGxMediaApiHelp = {
	.name  = "help",
	.func  = _gxmediaapi_help,
	.brief = "help: gxmediaapi help command",
};

int GxCore_Startup(int argc, char **argv)
{
	extern ShellCmd gGxMediaApi_PlayTs;
	extern ShellCmd gGxMediaApi_PlayEsa;
	extern ShellCmd gGxMediaApi_PlayEsv;
	extern ShellCmd gGxMediaApiStop;
	extern ShellCmd gGxMediaApiResume;
	extern ShellCmd gGxMediaApiPause;
	extern ShellCmd gGxMediaApiFinished;
	extern ShellCmd gGxMediaApiInfo;

	Log_SetModule(LOG_MEDIAAPI);
	Log_SetLevel (LOG_INFO|LOG_ERROR|LOG_WARNING);

	Shell_Init();

	Shell_AddCmd(&gGxMediaApi_PlayTs);
	Shell_AddCmd(&gGxMediaApi_PlayEsa);
	Shell_AddCmd(&gGxMediaApi_PlayEsv);
	Shell_AddCmd(&gGxMediaApiStop);
	Shell_AddCmd(&gGxMediaApiResume);
	Shell_AddCmd(&gGxMediaApiPause);
	Shell_AddCmd(&gGxMediaApiFinished);
	Shell_AddCmd(&gGxMediaApiInfo);

	Shell_AddCmd(&gGxMediaApiHelp);

	Shell_Start();

	Shell_Loop();

	Shell_Stop();

	return 0;
}
