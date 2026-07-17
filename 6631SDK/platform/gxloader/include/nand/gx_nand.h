#ifndef __GX_NAND_H__
#define __GX_NAND_H__

int gx_nand_init_v0(unsigned int base_addr, unsigned int is_boot);
int gx_nand_init_v1(unsigned int base_addr, unsigned int is_boot);
int gx_nand_readdata(uint32_t addr, uint8_t *data, uint32_t len);
char *gx_nand_gettype(void);
int gx_nand_getinfo(enum gx_flash_info flash_info);
int gx_nand_write_protect_lock(unsigned int protect_addr);
int gx_nand_write_protect_unlock(void);
void gx_nand_chiperase(void);
void gx_nand_scruberase(uint32_t addr, uint32_t len);
void gx_nand_blockerase(uint32_t addr, uint32_t len);
void gx_nand_blockerase_nospread(uint32_t addr, uint32_t len);
int gx_nand_pageprogram(uint32_t addr, uint8_t *data, uint32_t len);
void gx_nand_sync(void);
void gx_nand_calcblockrange(uint32_t addr, uint32_t len, uint32_t *pstart, uint32_t *pend);
int gx_nand_badinfo(void);
int gx_nand_pageprogram_yaffs2(uint32_t addr, uint8_t *data, uint32_t len);
int gx_nand_readoob(uint32_t addr, uint8_t *data, uint32_t len);
int gx_nand_writeoob(uint32_t addr, uint8_t *data, uint32_t len);
int gx_nand_readdata_noskip(uint32_t addr, uint8_t *data, uint32_t len);
void gx_nand_blockerase_noskip(uint32_t addr, uint32_t len);
int gx_nand_pageprogram_noskip(uint32_t addr, uint8_t *data, uint32_t len);
int gx_nand_writedata_noecc(unsigned int addr, unsigned char *data, unsigned int len);
int gx_nand_block_bbt_isbad(uint32_t addr);
int gx_nand_readdata_direct(uint32_t addr, uint8_t *data, uint32_t len);
int gx_nand_block_isbad(uint32_t addr);
int gx_nand_block_markbad(uint32_t addr);
int gx_nand_calc_phy_offset(uint32_t start, uint32_t logic_offset, uint32_t *phy_offset);
int gx_nand_read_data_and_oob(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);
int gx_nand_write_data_and_oob(uint32_t addr, uint8_t *data, uint32_t len, uint32_t oob_addr, uint8_t* oobbuf, uint32_t ooblen, int mode);

#endif
