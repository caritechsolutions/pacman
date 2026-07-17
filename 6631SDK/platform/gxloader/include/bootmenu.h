/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the GX3XXX boot loader */

#ifndef __BOOTLOADER_BOOTMENU_H
#define __BOOTLOADER_BOOTMENU_H

#define BOOTPROMPT          "boot> "

#define CONFIG_SYS_CBSIZE 256
#define CONFIG_CMDLINE_EDITING

// function prototype
int bootmenu(void);

// in loader.S
void do_boot(unsigned int addr) __attribute__ ((noreturn));

struct map_desc {
	unsigned long virtual;
	unsigned long physical;
	unsigned long length; /* NOTE: here the metric is MB */
	unsigned int type;
};

enum {
	MT_MEMORY = 0,
	MT_DEVICE,
};

extern char cmd_line_buffer[];

int cmd_line_readline(const char *const prompt);

#endif
