#ifndef __GX_FLASH_H__
#define __GX_FLASH_H__

#define FLASH_TYPE_NUM (3)

/*
 * flash ops function
 */
struct flash_op {
	int  (*init) (int verbose);
	int  (*readdata)(unsigned int addr, unsigned char *data, unsigned int len);
	void (*chiperase) (void);
	void (*scruberase) (unsigned int addr, unsigned int len);
	void (*erasedata)(unsigned int addr, unsigned int len);
	void (*erasedata_nospread)(unsigned int addr, unsigned int len);
	int  (*pageprogram)(unsigned int addr, unsigned char *data, unsigned int len);
	void (*sync)(void);
	void (*test)(int argc, char *argv[]);
	void (*calcblockrange)(unsigned int addr, unsigned int len, unsigned int *pstart, unsigned int *pend);
	int  (*badinfo)(void);
	int  (*pageprogram_yaffs2)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*readoob)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*writeoob)(unsigned int addr, unsigned char *data, unsigned int len);
	char *(*gettype)(void);
	int (*getinfo)(enum gx_flash_info flash_info);
	int (*write_protect_mode)(void);
	int (*write_protect_status)(unsigned int *len);
	int (*write_protect_lock)(unsigned int addr);
	int (*write_protect_unlock)(void);
	int (*set_lock_sr)(void);
	int (*get_lock_sr)(int *lock);
#ifdef CONFIG_ENABLE_FLASH_TEST
	int  (*readdata_noskip)(unsigned int addr, unsigned char *data, unsigned int len);
	void (*erasedata_noskip)(unsigned int addr, unsigned int len);
	int  (*pageprogram_noskip)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*pageprogram_noecc)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*block_bbt_isbad)(unsigned int addr);
	int  (*readdata_direct)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*read_data_and_oob)(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);
	int  (*write_data_and_oob)(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);
#endif
	int  (*otp_lock)(void);
	int  (*otp_status)(unsigned char *data);
	int  (*otp_erase)(void);
	int  (*otp_write)(unsigned int addr, unsigned char *data, unsigned int len);
	int  (*otp_read)(unsigned int addr, unsigned char* data, unsigned int len);
	int  (*otp_get_regions)(unsigned int *regions);
	int  (*otp_get_current_region)(unsigned int *region);
	int  (*otp_get_region_size)(unsigned int *size);
	int  (*otp_set_region)(unsigned int region);
	int (*block_isbad)(unsigned int addr);
	int (*block_markbad)(unsigned int addr);
	int (*calc_phy_offset)(uint32_t start, uint32_t logic_offset, uint32_t *phy_offset);
	int (*uid_read)(unsigned char *buf, int buf_len, int *ret_len);
};

#endif

