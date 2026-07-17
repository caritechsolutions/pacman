#ifndef __PARSER_H__
#define __PARSER_H__

#include "gxupdate_api.h"

bool image_version_is_ok(GxUpdateImageInfo *image, int print_version);
int parse_data(struct download_data *ddata,struct upgrade_data *udata, int max_num);
bool is_upgrade_all_bin(void);
struct partition *get_new_partition(void);

#endif
