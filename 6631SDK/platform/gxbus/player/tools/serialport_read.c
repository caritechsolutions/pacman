#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int cmd_help(int argc, char *argv[]);
static int cmd_get(int argc, char *argv[]);
static int cmd_set(int argc, char *argv[]);

static struct {
	char *cmd;
	int (*func)(int argc, char *argv[]);
	char *helpbrief;
} cmdtable[] = {
	{"help"     , cmd_help,     "help [cmd]"},
	{"get"      , cmd_get,      "get addr1 addr2 ..."},
	{"set"      , cmd_set,      "set addr1 value1 addr2 value2 ..."},

	{NULL, NULL, NULL}
};

static int cmd_help(int argc, char *argv[])
{
	int i;
	for (i = 0; cmdtable[i].cmd != NULL; ++i)
		if (cmdtable[i].helpbrief)
			gxlogd("\t%s\n", cmdtable[i].helpbrief);

	gxlogd("\tquit/exit/q\n");

	return 0;
}

static int cmd_get(int argc, char *argv[])
{
    int i;
    int addr;

    for(i = 1; i < argc; i ++)
    {
        sscanf(argv[i],"%x", &addr);
        gxlogd("[%s] : 0x%08x\n", argv[i], *(volatile int*)addr);
    }

    return 0;
}

static int cmd_set(int argc, char *argv[])
{
    int i;
    int addr, value;

    for(i = 1; i < argc; i +=2)
    {
        sscanf(argv[i]  , "%x", &addr);
        sscanf(argv[i+1], "%x", &value);
        *(volatile int*)addr = value;
    }

    gxlogd("\n--------set result---------\n");
    for(i = 1; i < argc; i +=2)
    {
        sscanf(argv[i]  , "%x", &addr);
        gxlogd("[%s] : 0x%08x\n", argv[i], *(volatile int*)addr);
    }

    return 0;
}

#ifndef LINUX_OS
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
	static char data[256];
	fputs(prompt, stdout);
	fflush(stdout);

	return fgets(data, 255, stdin);
}

static void add_history(char *line)
{

}
static int process_command(char *_str)
{
	int i, argc, cmdlen;
	char str[256], *cp;
	char *argv[256];

	strcpy(str, _str);
	trim(str);
	if (str[0] != 0) {
		if (strcasecmp(str, "exit") == 0 || strcasecmp(str, "q") == 0 || strcasecmp(str, "quit") == 0)
			return -2;

		memset(argv, 0, sizeof argv);
		argc = 0;
		for (cp = strtok(str, " "); cp != NULL; cp = strtok(NULL, " "))
			argv[argc++] = cp;

		// lookup the command table
		cmdlen = strlen(argv[0]);
		for (i = 0; cmdtable[i].cmd != NULL; ++i)
			if (strncmp(argv[0], cmdtable[i].cmd, cmdlen) == 0) {
				if (cmdtable[i].func(argc, argv) == -1)
					gxlogd("\tusage: %s\n", cmdtable[i].helpbrief);

				return 0;
			}

		gxlogd("not found command: %s\n", argv[0]);
	}

	return 0;
}
#endif


void player_serialport_read(void *data)
{
#ifndef LINUX_OS
	char *line;

	while( (line = readline(">> ")) != NULL) {
		add_history(line);
		if (process_command(line) == -2)
			break;
	}
#endif
}

