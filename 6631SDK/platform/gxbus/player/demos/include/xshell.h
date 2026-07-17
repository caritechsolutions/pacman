#ifndef __PLAYER_SHELL_H__
#define __PLAYER_SHELL_H__

typedef struct {
	char *name;
	int (*func)(int argc, char *argv[]);
	char *brief;
} ShellCmd;

typedef struct {
	ShellCmd **cmd;
	unsigned int max;
	unsigned int now;
} ShellTable;

int Shell_Init(void);

int Shell_AddCmd(ShellCmd *cmd);

int Shell_Start(void);

int Shell_Stop(void);

int Shell_Loop(void);

ShellTable *Shell_GetCmdTable(void);

#endif
