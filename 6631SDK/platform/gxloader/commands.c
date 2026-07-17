#include "stdio.h"
#include "string.h"

#include "config.h"
#include "gx_api.h"
#include "bootmenu.h"

#include "commands.h"
#include "bootconfig.h"
#include "time.h"
#include "serial.h"
#include "time.h"

COMMAND(help, command_new_help, "[command] -- Displays help text");
void command_new_help(int argc, const char **argv)
{
	struct bootldr_command *command;
	const char *search;
	if (argc <= 1) { // all help
		command = (struct bootldr_command *)&__commands_begin;
		while (command < (struct bootldr_command *)&__commands_end) {
			printf(command->helpstr);
			printf("\n");
			command++;
		}
	} else {
		if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) {
			search = argv[1];
		} else {
			search = argv[0];
		}
		command = (struct bootldr_command *)&__commands_begin;
		while (command < (struct bootldr_command *)&__commands_end) {
			if (strcmp(command->cmdstr, search) == 0) {
				printf(command->helpstr);
				printf("\n");
			}
			command++;
		}
	}
}

struct bootldr_command *get_command(const char *cmd)
{
	struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
		if (strcmp(cmd, command->cmdstr) == 0 && command->subcmd == NULL) {
			return command;
		}
		command++;
	}
	return NULL;
}

struct bootldr_command *get_sub_command(const char *cmd, const char *subcmd)
{
	struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
		if (command->subcmd != NULL && strcmp(cmd, command->cmdstr) == 0 && strcmp(subcmd, command->subcmd) == 0) {
			return command;
		}
		command++;
	}
	return get_command(cmd);
}

int has_sub_commands(const char *cmd)
{
	struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
		if (command->subcmd != NULL && strcmp(cmd, command->cmdstr) == 0) {
			return 1;
		}
		command++;
	}
	return 0;
}

int do_command(int argc, const char **argv)
{
	struct bootldr_command *command = NULL;

	if (strcmp(argv[argc - 1], "?") == 0 ||
			strcmp(argv[argc - 1], "help") == 0 ||
			strcmp(argv[0], "?") == 0 ||
			strcmp(argv[0], "help") == 0)
	{
		printf("Trying to load help...\n");
		command_new_help(argc, argv);
		return 1;
	}

	if (argc >= 2)
		command = get_sub_command(argv[0], argv[1]);
	if (command == NULL) command = get_command(argv[0]);

	if (command != NULL) {
		if (command->cmdfunc != NULL) {
			(*command->cmdfunc)(argc - command->offset, argv + command->offset);
			return 1;
		}
		return 0;
	}
	else {
		if (has_sub_commands(argv[0])) {
			argv[2] = NULL;
			argv[1] = argv[0];
			argv[0] = "help";
			argc = 2;
			command_new_help(argc, argv);
		} else {
			printf("Don't understand command ");
			printf(argv[0]);
			printf("\n");
			return 0;
		}
	}
	return 0;
}

static int abortboot(int bootdelay)
{
	int abort = 0;
	char tmp;

	if (bootdelay == -1)
		return 1;
	else if (bootdelay == 0)
		return abort;

	printf("Hit any key to stop autoboot:%2d ", bootdelay);
	while ((bootdelay > 0) && (!abort)) {
		int i;

		printf("\b\b\b%2d ", bootdelay);
		--bootdelay;
		for (i=0; !abort && i<100; ++i)
		{
			if(!uart_try_getc(&tmp, CONFIG_UART_PORT))
			{
				abort  = 1;          /* don't auto boot  */
				bootdelay = 0;       /* no more delay    */
				uart_try_getc(&tmp, CONFIG_UART_PORT); /* consume input */
				break;
			}

			udelay(10000);	// 10000us*100=1s
		}
	}

	puts("\n\n");

	return abort;
}

enum ParseState {
	PS_WHITESPACE,
	PS_TOKEN,
	PS_STRING,
	PS_ESCAPE
};

enum ParseState stackedState;
static void parseargs(char *argstr, int *argc_p, char **argv, char** resid)
{
	// const char *whitespace = " \t";
	int argc = 0;
	char c;
	enum ParseState lastState = PS_WHITESPACE;

	/* tokenize the argstr */
	while ((c = *argstr) != 0) {
		enum ParseState newState;

		if (c == ';' && lastState != PS_STRING && lastState != PS_ESCAPE)
			break;

		if (lastState == PS_ESCAPE) {
			newState = stackedState;
		} else if (lastState == PS_STRING) {
			if (c == '"') {
				newState = PS_WHITESPACE;
				*argstr = 0;
			} else {
				newState = PS_STRING;
			}
		} else if ((c == ' ') || (c == '\t')) {
			/* whitespace character */
			*argstr = 0;
			newState = PS_WHITESPACE;
		} else if (c == '"') {
			newState = PS_STRING;
			*argstr = 0;
			argv[argc++] = argstr + 1;
		} else if (c == '\\') {
			stackedState = lastState;
			newState = PS_ESCAPE;
		} else {
			/* token */
			if (lastState == PS_WHITESPACE) {
				argv[argc++] = argstr;
			}
			newState = PS_TOKEN;
		}

		lastState = newState;
		argstr++;
	}

	argv[argc] = NULL;
	if (argc_p != NULL)
		*argc_p = argc;

	if (*argstr == ';') {
		*argstr++ = '\0';
	}
	*resid = argstr;
}

void exec_string(char *buf)
{
	int argc;
	char *argv[128];
	char*	resid;
	int *status = (int*)(SRAMBASE + 0x400);

	while (*buf) {
		memset(argv, 0, sizeof(argv));
		parseargs(buf, &argc, argv, &resid);
		if (argc > 0) {
			*status = 0xA5A55A5A;
			do_command(argc, (const char **)argv);
			*status = 0x5A5AA5A5;
		} else {
			exec_string("help");
		}
		buf = resid;
	}
}


static void do_extern_cmd(void)
{
	char *auto_cmd;
	const char *extern_cmd = "extern_cmd";

	auto_cmd = (char *)(SRAMBASE + 0x200);

	if (strncmp(auto_cmd, extern_cmd, strlen(extern_cmd)) == 0) {
		printf("auto_cmd= %s\n", auto_cmd);
		auto_cmd += strlen(extern_cmd) + 1;
		exec_string(auto_cmd);
	}
}

int bootmenu(void)
{
	static char lastcommand[CONFIG_SYS_CBSIZE + 1] = { 0, };
	int len;
	int flag;
	int rc = 1;

	do_extern_cmd(); // run extern command, by jboot

	// if not abort run default command
	if (abortboot(g_bootconfig.delay) == 0)
		exec_string(g_bootconfig.autocmd);

	for (;;) {
		if (rc >= 0) {
			/* Saw enough of a valid command to
			 * restart the timeout.
			 */
			//bootretry_reset_cmd_timeout();
		}
		len = cmd_line_readline(BOOTPROMPT);

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strlcpy(lastcommand, cmd_line_buffer,
				CONFIG_SYS_CBSIZE + 1);
		else if (len == 0)
			flag |= 0;
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (len == -2) {
			/* -2 means timed out, retry autoboot
			 */
			puts("\nTimed out waiting for command\n");
# ifdef CONFIG_RESET_TO_RETRY
			/* Reinit board to run initialization code again */
			do_reset(NULL, 0, 0, NULL);
# else
			return;		/* retry autoboot */
# endif
		}
#endif

		if (len == -1)
			puts("<INTERRUPT>\n");
		else
			exec_string(lastcommand);

		rc = 1;

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}

	return 0;
}
