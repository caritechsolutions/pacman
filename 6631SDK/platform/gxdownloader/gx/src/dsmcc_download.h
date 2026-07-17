#ifndef __DSMCC_DOWNLOAD_H__
#define __DSMCC_DOWNLOAD_H__

#include "downloader.h"

enum GxOTA_DataStatus {
	GXOTA_DATA_RECEVING,
	GXOTA_DATA_FINISH,
	GXOTA_DATA_ERROR,
};

int dsmcc_download_data(struct download_data *ddata);

#endif
