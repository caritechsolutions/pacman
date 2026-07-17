/*
 * =====================================================================================
 *
 *       Filename:  flash.c
 *
 *    Description:  abstract the read/write/page/init for spi/nor/nandflash
 *
 *       Compiler:  gcc
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <config.h>
#include <stdio.h>
#include <gx_flash_common.h>
#include <flash.h>
#include "gx_flash.h"
#include "stdio.h"
#include "device.h"
#include "spi.h"
#include "misc.h"

static struct flash_op *flash_ops = NULL;

extern struct flash_op gxflash_nand_ops;
extern struct flash_op gxflash_spi_ops;
extern struct flash_op gxflash_nor_ops;
extern struct flash_op gxflash_spinand_ops;

inline int gxflash_init(void)
{
	int magic = 0;
	int ret     = -1;
	int cs = 0;

#if (defined(CONFIG_ENABLE_SFLASH) || defined(CONFIG_ENABLE_SPINAND))
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if ((adaptive.flash_type == FLASH_TYPE_NULL) || (adaptive.flash_type == FLASH_TYPE_SPI_NAND) || (adaptive.flash_type == FLASH_TYPE_SPI_NOR))
#endif
	{
		device_list_init();
		gx_spi_probe();
	}
#endif

#ifdef CONFIG_ENABLE_NANDFLASH
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if ((adaptive.flash_type == FLASH_TYPE_NULL) ||(adaptive.flash_type == FLASH_TYPE_NAND))
#endif
	{
		ret = gxflash_nand_ops.init(cs);
		if (ret >= 0) {
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
			adaptive.flash_type = FLASH_TYPE_NAND;
#endif
			flash_ops = &gxflash_nand_ops;
			goto exit;
		}
	}

#endif

#ifdef CONFIG_ENABLE_SPINAND
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if ((adaptive.flash_type == FLASH_TYPE_NULL) || (adaptive.flash_type == FLASH_TYPE_SPI_NAND))
#endif
	{
		ret = gxflash_spinand_ops.init(cs);
		if (ret >= 0) {
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
			adaptive.flash_type = FLASH_TYPE_SPI_NAND;
#endif
			flash_ops = &gxflash_spinand_ops;
			goto exit;
		}
	}
#endif

#ifdef CONFIG_ENABLE_SFLASH
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if ((adaptive.flash_type == FLASH_TYPE_NULL) || adaptive.flash_type == FLASH_TYPE_SPI_NOR)
#endif
	{
		ret = gxflash_spi_ops.init(cs);
		if (ret >= 0) {
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
			adaptive.flash_type = FLASH_TYPE_SPI_NOR;
#endif
			flash_ops = &gxflash_spi_ops;
			goto exit;
		}
	}
#endif

	if(ret == -1)
		printf("error: gxflash init failed.\n");

exit:
	return ret;
}

inline int gxflash_readdata(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->readdata != NULL)
		return flash_ops->readdata(addr, data, len);
	else
		return -1;
}

inline void gxflash_calc_phy_offset(unsigned int start, unsigned int logic_offset, unsigned int *phy_offset)
{
	if(flash_ops->calc_phy_offset != NULL)
		flash_ops->calc_phy_offset(start, logic_offset, phy_offset);
	else
		*phy_offset = logic_offset;
}

inline void gxflash_chiperase (void)
{
	if(flash_ops->chiperase != NULL)
		flash_ops->chiperase();
}

inline void gxflash_erasedata(unsigned int addr, unsigned int len)
{
	if(flash_ops->erasedata != NULL)
		flash_ops->erasedata(addr, len);
}

inline void gxflash_erasedata_nospread(unsigned int addr, unsigned int len)
{
	if(flash_ops->erasedata != NULL)
		flash_ops->erasedata_nospread(addr, len);
}


inline int gxflash_pageprogram(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->pageprogram != NULL)
		return flash_ops->pageprogram(addr, data, len);
	else
		return -1;
}

inline void gxflash_sync(void)
{
	if(flash_ops->sync != NULL)
		flash_ops->sync();
}

inline void gxflash_scruberase(unsigned int addr, unsigned int len)
{
	if(flash_ops->scruberase != NULL)
		flash_ops->scruberase(addr, len);
}

#ifdef CONFIG_ENABLE_FLASH_TEST
inline int gxflash_readdata_noskip(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->readdata_noskip != NULL)
		return flash_ops->readdata_noskip(addr, data, len);
	else
		return -1;
}

inline void gxflash_erasedata_noskip(unsigned int addr, unsigned int len)
{
	if(flash_ops->erasedata_noskip != NULL)
		flash_ops->erasedata_noskip(addr, len);
}

inline int gxflash_pageprogram_noskip(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->pageprogram_noskip != NULL)
		return flash_ops->pageprogram_noskip(addr, data, len);
	else
		return -1;
}

inline int gxflash_pageprogram_noecc(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->pageprogram_noecc != NULL)
		return flash_ops->pageprogram_noecc(addr, data, len);
	else
		return -1;
}

inline int gxflash_block_bbt_isbad(unsigned int addr)
{
	if (flash_ops->block_bbt_isbad!= NULL)
		return flash_ops->block_bbt_isbad(addr);
	else
		return -1;
}

inline int gxflash_readdata_direct(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->readdata_direct != NULL)
		return flash_ops->readdata_direct(addr, data, len);
	else
		return -1;
}

inline int gxflash_read_data_and_oob(unsigned int addr, unsigned char *data, unsigned int len, unsigned int oob_addr, unsigned char* oobbuf, unsigned int ooblen, unsigned int mode)
{
	if(flash_ops->read_data_and_oob != NULL)
		return flash_ops->read_data_and_oob(addr, data, len, oob_addr, oobbuf, ooblen, mode);
	else
		return -1;
}

inline int gxflash_write_data_and_oob(unsigned int addr, unsigned char *data, unsigned int len, unsigned int oob_addr, unsigned char* oobbuf, unsigned int ooblen, unsigned int mode)
{
	if(flash_ops->write_data_and_oob != NULL)
		return flash_ops->write_data_and_oob(addr, data, len, oob_addr, oobbuf, ooblen, mode);
	else
		return -1;
}

#endif

inline void gxflash_test(int argc, char *argv[])
{
	if(flash_ops->test != NULL)
		flash_ops->test(argc, argv);
}

inline void gxflash_calcblockrange(unsigned int addr, unsigned int len, unsigned int *pstart, unsigned int *pend)
{
	if(flash_ops->calcblockrange != NULL)
		flash_ops->calcblockrange(addr, len, pstart, pend);
}

inline int gxflash_badinfo(void)
{
	if(flash_ops->badinfo != NULL)
		return flash_ops->badinfo();
	else
		return -1;
}

inline int gxflash_pageprogram_yaffs2(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->pageprogram_yaffs2 != NULL)
		return flash_ops->pageprogram_yaffs2(addr, data, len);
	else {
		printf("func(%s) is NULL, please check.\n", __func__);
		return -1;
	}
}

inline int gxflash_readoob(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->readoob != NULL)
		return flash_ops->readoob(addr, data, len);
	else
		return -1;
}

inline int gxflash_writeoob(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->writeoob != NULL)
		return flash_ops->writeoob(addr, data, len);
	else
		return -1;
}

char * gxflash_gettype(void)
{
	if(flash_ops->gettype != NULL)
		return flash_ops->gettype();
	else
		return NULL;
}

int gxflash_get_info(enum gx_flash_info flash_info)
{
	if(flash_ops->getinfo!= NULL)
		return flash_ops->getinfo(flash_info);
	else
		return -1;
}

int gxflash_write_protect_mode(void)
{
	if(flash_ops->write_protect_mode != NULL)
		return flash_ops->write_protect_mode();
	else
		return -1;
}

int gxflash_write_protect_status(unsigned int *len)
{
	if(flash_ops->write_protect_status != NULL)
		return flash_ops->write_protect_status(len);
	else
		return -1;
}

int gxflash_write_protect_lock(unsigned long addr)
{
	if(flash_ops->write_protect_lock != NULL)
		return flash_ops->write_protect_lock(addr);
	else
		return -1;
}

int gxflash_write_protect_unlock(void)
{
	if(flash_ops->write_protect_unlock != NULL)
		return flash_ops->write_protect_unlock();
	else
		return -1;
}

int gxflash_set_lock_sr(void)
{
	if(flash_ops->set_lock_sr!= NULL)
		return flash_ops->set_lock_sr();
	else
		return -1;
}

int gxflash_get_lock_sr(int *lock)
{
	if(flash_ops->get_lock_sr!= NULL)
		return flash_ops->get_lock_sr(lock);
	else
		return -1;
}

void flash_switch_type(int flag)
{
	int ret = -1;
	int cs = 0;

	if (flag == flash_get_type())
		return;

	switch(flag){
		case 0:
#ifdef CONFIG_ENABLE_SFLASH
			ret = gxflash_spi_ops.init(cs);
			if(ret >= 0){
				flash_ops = &gxflash_spi_ops;
				printf("flash driver switch to spinor flash.\n");
			}else
				printf("error: flash driver switch to spinor failed.\n");
#else
			printf("not enable spi nor flash, can't switch.\n");
#endif
			break;
		case 1:
#ifdef CONFIG_ENABLE_SPINAND
			ret = gxflash_spinand_ops.init(cs);
			if(ret >= 0){
				flash_ops = &gxflash_spinand_ops;
				printf("flash driver switch to spinand flash.\n");
			}else
				printf("error: flash driver switch to spinand failed.\n");
#else
			printf("not enable spi nand flash, can't switch.\n");
#endif
			break;
		case 2:
#ifdef CONFIG_ENABLE_NANDFLASH
			ret = gxflash_nand_ops.init(cs);
			if(ret >= 0){
				flash_ops = &gxflash_nand_ops;
				printf("flash driver switch to nand flash.\n");
			}else
				printf("error: flash driver switch to nand failed.\n");
#else
			printf("not enable nand flash, can't switch.\n");
#endif
			break;
		case 3:
#if defined(CONFIG_ENABLE_SFLASH ) && defined(CONFIG_EXTEND_MTDPARTS)
			cs = 2;
			ret = gxflash_spi_ops.init(cs);
			if(ret >= 0){
				flash_ops = &gxflash_spi_ops;
				printf("flash driver switch to spinor extend flash.\n");
			}else
				printf("error: flash driver switch to spinor failed.\n");
#else
			printf("not enable spi nor flash or extend nor flash, can't switch.\n");
#endif
			break;
		default:
			printf("flash switch flag not support.\n");
			break;
	}
}

int flash_get_type(void)
{
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if (adaptive.flash_type > 0)
		return adaptive.flash_type - 1;
#else
#ifdef CONFIG_ENABLE_SFLASH
	if(flash_ops == &gxflash_spi_ops)
		return 0;
#endif

#ifdef CONFIG_ENABLE_SPINAND
	if(flash_ops == &gxflash_spinand_ops)
		return 1;
#endif

#ifdef CONFIG_ENABLE_NANDFLASH
	if(flash_ops == &gxflash_nand_ops)
		return 2;
#endif
#endif
	printf("error: flash get type failed.\n");
	return -1;
}

int flash_get_used_device_map(char *map)
{
	if (map == NULL) {
		printf("error: point is null when get flash device map!");
		return -1;
	}

	memset(map, 0, FLASH_TYPE_NUM);
#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if (adaptive.flash_type > 0)
		map[adaptive.flash_type - 1] = 1;
#else
#ifdef CONFIG_ENABLE_SFLASH
	map[0] = 1;
#endif
#ifdef CONFIG_ENABLE_SPINAND
	map[1] = 1;
#endif
#ifdef CONFIG_ENABLE_NANDFLASH
	map[2] = 1;
#endif
#endif

	return 0;
}

int gxflash_block_isbad(unsigned int addr)
{
	if (flash_ops->block_isbad != NULL)
		return flash_ops->block_isbad(addr);
	else
		return -1;
}

int gxflash_block_markbad(unsigned int addr)
{
	if (flash_ops->block_markbad != NULL)
		return flash_ops->block_markbad(addr);
	else
		return -1;
}


int gxflash_otp_lock(void)
{
	if(flash_ops->otp_lock != NULL)
		return flash_ops->otp_lock();
	else
		return -1;
}

int gxflash_otp_status(unsigned char *data)
{
	if(flash_ops->otp_status != NULL)
		return flash_ops->otp_status(data);
	else
		return -1;
}

int gxflash_otp_erase(void)
{
	if(flash_ops->otp_erase != NULL)
		return flash_ops->otp_erase();
	else
		return -1;
}

int gxflash_otp_write(unsigned int addr, unsigned char *data, unsigned int len)
{
	if(flash_ops->otp_write != NULL)
		return flash_ops->otp_write(addr, data, len);
	else
		return -1;
}

int gxflash_otp_read(unsigned int addr, unsigned char* data, unsigned int len)
{
	if(flash_ops->otp_read != NULL)
		return flash_ops->otp_read(addr, data, len);
	return -1;
}

int gxflash_otp_get_region(unsigned int *region)
{
	if(flash_ops->otp_get_regions != NULL)
		return flash_ops->otp_get_regions(region);
	else
		return -1;
}

int gxflash_otp_set_region(unsigned int region)
{
	if(flash_ops->otp_set_region != NULL)
		return flash_ops->otp_set_region(region);
	else
		return -1;
}

int gxflash_otp_get_current_region(unsigned int *region)
{
	if(flash_ops->otp_get_current_region != NULL)
		return flash_ops->otp_get_current_region(region);
	else
		return -1;
}

int gxflash_otp_get_region_size(unsigned int *size)
{
	if(flash_ops->otp_get_region_size != NULL)
		return flash_ops->otp_get_region_size(size);
	else
		return -1;
}

int gxflash_uid_read(unsigned char* data, unsigned int len, unsigned int *ret_len)
{
	if(flash_ops->uid_read != NULL)
		return flash_ops->uid_read(data, len, ret_len);
	return -1;
}
