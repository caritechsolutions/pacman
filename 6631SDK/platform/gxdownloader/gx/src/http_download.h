#ifndef __HTTP_DOWNLOAD_H__
#define __HTTP_DOWNLOAD_H__

#include "downloader.h"
#include "gxtype.h"

int http_download_data(struct download_data *ddata);
void http_upgrade_set_install_flag(char flag);
bool http_download_is_stop(void);

#endif
