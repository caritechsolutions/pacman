#ifndef __GXFRONTEND_NET_H__
#define __GXFRONTEND_NET_H__

#include <fcntl.h>
#include "gxcore.h"

typedef struct {
	int32_t (*open)(const char* url);
	int32_t (*close)(void);
	int32_t (*read_status)(uint32_t* status);
	int32_t (*start_get_data)(void);
	int32_t (*stop_get_data)(void);
	int32_t (*pause_get_data)(void);
	int32_t (*continue_get_data)(void);
}GxFrontendNetOps;


int32_t GxFrontendNet_Register(char *player_name);


#endif

