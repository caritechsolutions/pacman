/*
 * =====================================================================================
 *
 *       Filename:  flash.h
 *
 *    Description:  the common interface for spi/nor/nandflash
 *
 *       Compiler:  gcc
 *
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __FLASH_H__
#define __FLASH_H__

/*
 * flash_ops interface function
 */
int  gxflash_init(void);
int  gxflash_readdata(unsigned int addr, unsigned char *data, unsigned int len);
void gxflash_calc_phy_offset(unsigned int start, unsigned int logic_offset, unsigned int *phy_offset);
void gxflash_chiperase (void);
void gxflash_erasedata(unsigned int addr, unsigned int len);
void gxflash_erasedata_nospread(unsigned int addr, unsigned int len);
int  gxflash_pageprogram(unsigned int addr, unsigned char *data, unsigned int len);
void gxflash_sync(void);
void gxflash_test(int argc, char *argv[]);
void gxflash_calcblockrange(unsigned int addr, unsigned int len, unsigned int *pstart, unsigned int *pend);
int  gxflash_badinfo(void);
int  gxflash_pageprogram_yaffs2(unsigned int addr, unsigned char *data, unsigned int len);
int  gxflash_readoob(unsigned int addr, unsigned char *data, unsigned int len);
int  gxflash_writeoob(unsigned int addr, unsigned char *data, unsigned int len);
char * gxflash_gettype(void);
int  gxflash_get_info(enum gx_flash_info flash_info);
int  gxflash_write_protect_mode(void);
int  gxflash_write_protect_status(unsigned int *len);
int  gxflash_write_protect_lock(unsigned long addr);
int  gxflash_write_protect_unlock(void);

int gxflash_set_lock_sr(void);
int gxflash_get_lock_sr(int *lock);

extern int flash_complete_test(unsigned int count);
extern void flash_oob_test(void);

#ifdef CONFIG_ENABLE_FIREWALL
extern void flash_firewall_test(unsigned int ram_addr, unsigned int ram_size, int read_permission, int write_permission);
#endif

extern int flash_sample_delay_test(unsigned int div);

void flash_switch_type(int flag);
int flash_get_type(void);

void gxflash_scruberase(unsigned int addr, unsigned int len);
int gxflash_readdata_noskip(unsigned int addr, unsigned char *data, unsigned int len);
void gxflash_erasedata_noskip(unsigned int addr, unsigned int len);
int gxflash_pageprogram_noskip(unsigned int addr, unsigned char *data, unsigned int len);
int gxflash_pageprogram_noecc(unsigned int addr, unsigned char *data, unsigned int len);

int flash_get_used_device_map(char *map);
int gxflash_block_isbad(unsigned int addr);
int gxflash_block_markbad(unsigned int addr);

int gxflash_otp_lock(void);
int gxflash_otp_status(unsigned char *data);
int gxflash_otp_erase(void);
int gxflash_otp_write(unsigned int addr, unsigned char *data, unsigned int len);
int gxflash_otp_read(unsigned int addr, unsigned char* data, unsigned int len);
int gxflash_otp_get_region(unsigned int *region);
int gxflash_otp_set_region(unsigned int region);
int gxflash_otp_get_current_region(unsigned int *region);
int gxflash_otp_get_region_size(unsigned int *size);

int gxflash_uid_read(unsigned char* data, unsigned int len, unsigned int *ret_len);

#endif

