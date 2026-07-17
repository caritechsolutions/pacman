#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "gxos/gxcore_os_core.h"
#include "xshell.h"
#include "log.h"

static ShellTable _sShellTable;
static int shell_exit_c = 0;
static handle_t shell_thread_id = 0;
static int shell_thread_running = 0;

static char *trim(char *str)
{
	char *cp = str;

	while (*cp != 0 && isspace(*cp))
		++cp;

	if (cp == str)
		cp += strlen(str) - 1;
	else {
		char *cpdes;

		for (cpdes = str; *cp != 0; )
			*cpdes++ = *cp++;
		cp = cpdes;
		*cp-- = 0;
	}

	while (cp > str && isspace(*cp))
		*cp-- = 0;

	return str;
}

static char *readline(char *prompt)
{
	char c = 0;
	unsigned int  i = 0;
	static char data[256];

	fputs(prompt, stdout);
	fflush(stdout);

	while (1) {
		c = fgetc(stdin);
		if (c == '\r' || c == '\n')
			break;
		data[i] = c;
		i++;
	}

	data[i] = '\0';

	return data;
}

static int shell_cmd_process(char *_str)
{
	int i = 0, argc, cmdlen;
	char str[256], *cp;
	char *argv[256];

	strcpy(str, _str);
	trim(str);

	if (str[0] != 0) {
		if (strcasecmp(str, "exit") == 0)
			return -2;

		memset(argv, 0, sizeof argv);
		argc = 0;
		for (cp = strtok(str, " "); cp != NULL; cp = strtok(NULL, " "))
			argv[argc++] = cp;

		// lookup the command table
		cmdlen = strlen(argv[0]);
		for (i = 0; i < _sShellTable.now; ++i) {
			if (strncmp(argv[0], _sShellTable.cmd[i]->name, cmdlen) == 0) {
				_sShellTable.cmd[i]->func(argc, argv);
				return 0;
			}
		}
	}

	if (i == _sShellTable.now) {
		for (i = 0; i < _sShellTable.now; ++i) {
			if (strncmp("help", _sShellTable.cmd[i]->name, strlen("help")) == 0) {
				_sShellTable.cmd[i]->func(0, NULL);

				return 0;
			}
		}
	}

	return 0;
}

static void shell_cmd_thread(void *data)
{
	while (shell_thread_running) {
		char *line = readline(">> ");
		if (line == NULL)
			continue;

		if (shell_exit_c)
			break;

		if (shell_cmd_process(line) == -2) {
			shell_exit_c = 1;
			break;
		}
	}

	return;
}

static void shell_handler(int sig)
{
	shell_exit_c = 1;
}

int Shell_Init(void)
{
	_sShellTable.now = 0;
	_sShellTable.max = 0;
	_sShellTable.cmd = NULL;

	return 0;
}

int Shell_AddCmd(ShellCmd *cmd)
{
	if (_sShellTable.now == _sShellTable.max) {
		_sShellTable.max += 16;
		_sShellTable.cmd = realloc(_sShellTable.cmd, (sizeof(ShellCmd *) * _sShellTable.max));
	}

	_sShellTable.cmd[_sShellTable.now] = cmd;
	_sShellTable.now++;

	return 0;
}

ShellTable *Shell_GetCmdTable(void)
{
	return &_sShellTable;
}

int Shell_Start(void)
{
	shell_thread_running = 1;

	if (GxCore_ThreadCreate("shell thread",
				&shell_thread_id, shell_cmd_thread,
				NULL, 32*1024, GXOS_DEFAULT_PRIORITY - 1)) {
		Log_Printf(LOG_COMMON, LOG_ERROR, "%s %d\n", __func__, __LINE__);
		exit(1);
	}

	return 0;
}

int Shell_Stop(void)
{
	shell_thread_running = 0;
	GxCore_ThreadJoin(shell_thread_id);

	return 0;
}

int Shell_Loop(void)
{
	signal(SIGINT, shell_handler);

	while (!shell_exit_c) {
		GxCore_ThreadDelay(100);
	}

	return 0;
}
