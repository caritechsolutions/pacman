/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the boot loader */

#ifndef __ROMFSLOADER_ROMFS_H
#define __ROMFSLOADER_ROMFS_H

// 
// function ptototypes
//

int boot_romfs(char *name, unsigned int kload, unsigned int addr, unsigned int to);
int dump_romfs(unsigned int addr, unsigned int to);
unsigned int load_romfs_file(char *name, unsigned int load_addr, unsigned int addr, unsigned int to, int autoexpand);
unsigned int find_romfs(unsigned int addr, unsigned int to);
unsigned int romfs_size(unsigned int *addr);

#endif
