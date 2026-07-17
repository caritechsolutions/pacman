#define CHTTPD_VERSION "0.1"

#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "webserver.h"
#include "debug.h"
#include "chttpdconfig.h"

using namespace std;

CWebserver *webserver;
extern chttpdConfig cfg;

extern "C" char *process_command(char *cmd_str)
{
	return NULL;
}

#if 0
int main(int argc, char **argv)
{
	CDEBUG::getInstance()->Debug = true;

	webserver = new CWebserver();

	webserver->Wait();

	return 0;
}

#endif
