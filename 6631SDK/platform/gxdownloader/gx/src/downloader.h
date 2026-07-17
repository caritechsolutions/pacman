#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include "gxdlp_api.h"

struct download_data{
	unsigned char name[20];
	unsigned char *data;
	unsigned int   len;
};

int download_data(struct download_data *ddata, GxDLP_Download_Type download_type);

#endif
