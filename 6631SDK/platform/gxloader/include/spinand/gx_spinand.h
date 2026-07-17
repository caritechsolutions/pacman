#ifndef __GX_SPINAND_H__
#define __GX_SPINAND_H__

int gx_spinand_init(int bus_num,int cs, int speed, int mode, int is_boot);
int gx_spinand_readdata(uint32_t addr, uint8_t *data, uint32_t len);
int gx_spinand_calc_phy_offset(uint32_t start, uint32_t logic_offset, uint32_t *phy_offset);
int gx_spinand_writedata(unsigned int addr, unsigned char *data, unsigned int len);
void gx_spinand_blockerase(uint32_t addr, uint32_t len);
void gx_spinand_blockerase_nospread(uint32_t addr, uint32_t len);
void gx_spinand_chiperase(void);
void gx_spinand_scruberase(uint32_t addr, uint32_t len);
int gx_spinand_readdata_noskip(uint32_t addr, uint8_t *data, uint32_t len);
int gx_spinand_writedata_noskip(unsigned int addr, unsigned char *data, unsigned int len);
void gx_spinand_blockerase_noskip(uint32_t addr, uint32_t len);
int gx_spinand_writedata_noecc(unsigned int addr, unsigned char *data, unsigned int len);
int gx_spinand_readdata_direct(uint32_t addr, uint8_t *data, uint32_t len);
int gx_spinand_readoob(uint32_t addr, uint8_t *data, uint32_t len);
int gx_spinand_writeoob(uint32_t addr, uint8_t *data, uint32_t len);
char *gx_spinand_gettype(void);
int gx_spinand_getinfo(enum gx_flash_info flash_info);
void gx_spinand_calcblockrange(uint32_t addr, uint32_t len, uint32_t *pstart, uint32_t *pend);
int gx_spinand_badinfo(void);
int gx_spinand_pageprogram_yaffs2(uint32_t addr, uint8_t *data, uint32_t len);
int gx_spinand_block_isbad(uint32_t addr);
int gx_spinand_block_bbt_isbad(uint32_t addr);
int gx_spinand_block_markbad(uint32_t addr);
int gx_spinand_write_protect_lock(unsigned int protect_len);
int gx_spinand_write_protect_unlock(void);
int gx_spinand_read_uniqueid(unsigned char* buf, int buf_len,int *ret_len);
int gx_spinand_otp_status(unsigned char *buf);
int gx_spinand_otp_lock(void);
int gx_spinand_otp_read(unsigned int addr, unsigned char *buf, unsigned int len);
int gx_spinand_otp_write(unsigned int addr, unsigned char *buf, unsigned int len);
int gx_spinand_otp_get_regions(unsigned int *regions);
int gx_spinand_otp_set_region(unsigned int region);
int gx_spinand_otp_get_region_size(unsigned int *size);
int gx_spinand_otp_get_current_region(unsigned int *region);
int gx_spinand_read_data_and_oob(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);
int gx_spinand_write_data_and_oob(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);

#endif
