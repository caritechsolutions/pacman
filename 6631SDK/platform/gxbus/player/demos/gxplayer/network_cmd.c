#include "xshell.h"
#include "log.h"
#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char *sNetworkTestOP = "t:i:p:s:";
static void _network_test_help(void)
{
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------INFO COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: network -t <protocol>\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -t : udp/rtp/tcp\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -s : 1-stop/0-start\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -i : ip addresss\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -p : ip port\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
}

static void _network_init(void)
{
	static int s_network_init = 0;

	if (s_network_init == 0) {
#ifdef ECOS_OS
		extern void network_init(char *if_name);
		extern int  network_dhcp(char *if_name);
		network_init("eth0");
		network_dhcp("eth0");
#else
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "please check network is start..\n", __func__, __LINE__);
#endif
		s_network_init = 1;
	}
}

static int _network_test(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt, ret;
	const char *op_string = NULL;
	const char *op_ip = 0;
	int op_port = 0;
	int op_stopflag = 0;

	_network_init();

	optind = 0;
	while ((opt = GetOpt(argc, argv, sNetworkTestOP)) != -1) {
		switch (opt) {
		case 't':
			op_string = strdup(optarg);
			break;
		case 's':
			op_stopflag = atoi(optarg);
			break;
		case 'i':
			op_ip = strdup(optarg);
			break;
		case 'p':
			op_port = atoi(optarg);
			break;
		default:
			_network_test_help();
			return 0;
		}
	}

	if (!op_stopflag &&
			(op_string == NULL || op_ip == NULL)) {
		_network_test_help();
		return 0;
	}

	if (strcmp(op_string, "rtp") == 0) {
		if (op_stopflag) {
			extern int GxRTP_Test_Stop(void);
			GxRTP_Test_Stop();
		} else {
			extern int GxRTP_Test_Start(const char *ip, int port);
			GxRTP_Test_Start(op_ip, op_port);
		}
	} else if (strcmp(op_string, "tcp") == 0) {
	} else if (strcmp(op_string, "udp") == 0) {
	} else
		_network_test_help();
	if (op_string)
		free((void*)op_string);
	if (op_ip)
		free((void*)op_ip);

	return 0;
}

ShellCmd gNetworkTest = {
	.name  = "net",
	.func  = _network_test,
	.brief = "net   [help]: network info command",
};
