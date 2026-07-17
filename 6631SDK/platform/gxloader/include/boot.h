/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the GX3XXX boot loader */

#ifndef __BOOTLOADER_BOOT_H
#define __BOOTLOADER_BOOT_H
#include "partition.h"

struct signature_ops_t {
	int (*get_signature)(struct partition_info *partition, unsigned char **signature, unsigned int *len);
	int (*verify_signature)(struct partition_info *partition, unsigned char *signature, unsigned int sign_len, unsigned char *src, unsigned int src_len);
};

struct crypt_ops_t {
	int (*app_dec)(struct partition_info *partition, unsigned char *src, unsigned int src_len, unsigned char *dst, unsigned int dst_len);
};

void simple_boot(int argc, const char **argv);
void run_kernel(u32 addr, unsigned long dtb_addr);
int doboot_kernel(struct partition_info *partition);
void gx_show_logo(void);
int memsize_parse(unsigned long *p_act_size);
void cmdline_init(const char *cmd);
void cmdline_clear(void);
char * get_cmdline(void);
unsigned long cmdline_get_boot_params(void);
void update_cmdline(const char *cmd, struct partition_table_info *table_info);
void cmdline_add(const char *cmd);
void cmdline_add_noappend_space(const char *cmd);
void mtdparts_to_cmdline(void);
void mtdparts_cryptinfo_to_cmdline(const char *crypt_info);
void register_signature(struct signature_ops_t *sign_ops);
void register_decrypt(struct crypt_ops_t *crypt_ops);
#endif				// __BOOTLOADER_BOOT_H
