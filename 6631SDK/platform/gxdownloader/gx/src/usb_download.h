#ifndef __USB_DOWNLOAD_H__
#define __USB_DOWNLOAD_H__

#include "downloader.h"

int usb_download_data(struct download_data *udata);
int usb_get_dir(char* dir,void* pbuf,unsigned long maxsize,unsigned long* file_len);

#endif
