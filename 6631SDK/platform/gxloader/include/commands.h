#ifndef _COMMANDS_H_
#define _COMMANDS_H_

struct bootldr_command {
	const char *cmdstr;
	const char *subcmd;
	void (*cmdfunc)(int argc, const char **);
	const char *helpstr;
	int offset;   /* number of args to remove from the left side */
};

#define __commanddata	__attribute__ ((__section__ (".data.commands")))

#define COMMAND(cmd_, func_, help_) \
	void func_(int argc, const char **); \
struct bootldr_command __commanddata bootldr_cmd_ ## cmd_ = { #cmd_, NULL, func_, #cmd_ " " help_, 0 }

#define SUBCOMMAND(cmd_, sub_, func_, help_, offset_) \
	void func_(int argc, const char **); \
struct bootldr_command __commanddata bootldr_cmd_ ## cmd_ ## sub_ = { #cmd_, #sub_, func_, #cmd_ " " #sub_ " " help_, offset_ }

#define CMD(cmd_) \
	&bootldr_cmd_ ## cmd_

#define SUBCMD(cmd_, sub_) \
	&bootldr_cmd_ ## cmd_ ## sub_

extern struct bootldr_command __commands_begin;
extern struct bootldr_command __commands_end;

extern struct bootldr_command *get_command(const char *cmd);
extern struct bootldr_command *get_sub_command(const char *cmd, const char *subcmd);
extern int do_command(int argc, const char **argv);
extern int getc(void);
void command_init(void);

#endif // _COMMANDS_H_
