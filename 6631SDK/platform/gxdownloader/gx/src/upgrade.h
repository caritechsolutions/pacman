#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#include "downloader.h"

#define MAX_FILE 32

struct upgrade_data {
	unsigned char    *src;
	unsigned int     size;
	unsigned int     dst;
	char             name[16];
	int              partition_pos;
	struct partition_info      *partition;
	bool             write_back;
};

void upgrade(void);
int acquire_data(struct download_data *ddata,struct upgrade_data *udata, int max_num);
int update_data(struct upgrade_data* udata, int num);
int check_partition_status(int show_error);
#endif
