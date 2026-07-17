#include <config.h>
#include <sys/types.h>
#include <driver/gx_flash_common.h>
#include "flash.h"
#include "partition.h"
#include <linux/mtd/mtd.h>
#include <linux/mtd/mtd-abi.h>

extern struct partition *flash_entry_arr[];
static struct mtd_info *mtd_info = NULL;
static int mtd_num = 0;


#define PAGE_MAIN_SIZE_2K          0x800
#define PAGE_MAIN_SIZE_4K          0x1000
#define PAGE_NUM_PER_BLOCK_64      64
#define BLOCK_MAIN_SIZE_128K       (PAGE_MAIN_SIZE_2K * PAGE_NUM_PER_BLOCK_64)
#define BLOCK_MAIN_SIZE_256K       (PAGE_MAIN_SIZE_4K * PAGE_NUM_PER_BLOCK_64)

#define BLOCK_SHIFT_128K           (17)
#define BLOCK_SHIFT_256K           (18)
#define BLOCK_MASK_128K            (BLOCK_MAIN_SIZE_128K - 1)
#define BLOCK_MASK_256K            (BLOCK_MAIN_SIZE_256K - 1)
#define PAGE_SHIFT_2K              (11)
#define PAGE_SHIFT_4K              (12)
#define PAGE_MASK_2K               (PAGE_MAIN_SIZE_2K - 1)
#define PAGE_MASK_4K               (PAGE_MAIN_SIZE_4K - 1)

int mtd_probe_devices(void)
{
	struct partition *flash_entry;
	int i;

	i = flash_get_type();
	flash_entry = flash_entry_arr[i];
	mtd_num = flash_entry->count;
	if (mtd_info)
		free(mtd_info);
	mtd_info = (struct mtd_info*)malloc(sizeof(struct mtd_info) * mtd_num);
	if (mtd_info == NULL) {
		mtd_num = 0;
		printf("%s: %d malloc failed\n", __func__, __LINE__);
		return -1;
	}
	memset(mtd_info, 0, sizeof(struct mtd_info) * mtd_num);

	for (i = 0; i < mtd_num; i++) {
		mtd_info[i].type = gxflash_get_info(GX_FLASH_CHIP_TYPE) == \
				 GX_FLASH_CHIP_TYPE_NOR ? MTD_NORFLASH : MTD_NANDFLASH;
		mtd_info[i].flags = MTD_WRITEABLE;
		mtd_info[i].size = flash_entry->tables[i].total_size;

		mtd_info[i].erasesize = gxflash_get_info(GX_FLASH_ERASE_SIZE);
		mtd_info[i].writesize = gxflash_get_info(GX_FLASH_PAGE_SIZE);
		mtd_info[i].writebufsize = gxflash_get_info(GX_FLASH_PAGE_SIZE);
		mtd_info[i].oobsize = 0;
		mtd_info[i].oobavail = 0;

		mtd_info[i].erasesize_shift = mtd_info[i].erasesize == BLOCK_MAIN_SIZE_128K ? BLOCK_SHIFT_128K : BLOCK_SHIFT_256K;
		mtd_info[i].writesize_shift = mtd_info[i].writesize == PAGE_MAIN_SIZE_2K ? PAGE_SHIFT_2K : PAGE_SHIFT_4K;
		mtd_info[i].bitflip_threshold = 0xff;
		mtd_info[i].name = flash_entry->tables[i].name;
		mtd_info[i].index = i;
		mtd_info[i]._block_isbad = mtd_block_isbad;

		mtd_info[i].offset = flash_entry->tables[i].start_addr;
	}

	return 0;
}

/**
 *	get_mtd_device - obtain a validated handle for an MTD device
 *	@mtd: last known address of the required MTD device
 *	@num: internal device number of the required MTD device
 *
 *	Given a number and NULL address, return the num'th entry in the device
 *	table, if any.	Given an address and num == -1, search the device table
 *	for a device with that address and return if it's still present. Given
 *	both, return the num'th driver only if its address matches. Return
 *	error code if not.
 */
struct mtd_info *get_mtd_device(struct mtd_info *mtd, int num)
{
	if (mtd_info == NULL)
		return NULL;
	else
		return &mtd_info[num];
}


/**
 *	get_mtd_device_nm - obtain a validated handle for an MTD device by
 *	device name
 *	@name: MTD device name to open
 *
 *	This function returns MTD device description structure in case of
 *	success and an error code in case of failure.
 */
struct mtd_info *get_mtd_device_nm(const char *name)
{
	int i;

	for (i = 0; i < mtd_num; i++) {
		if (strcmp(mtd_info[i].name, name) == 0) {
			return &mtd_info[i];
		}
	}

	return NULL;
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (!instr->len) {
		instr->state = MTD_ERASE_DONE;
		return 0;
	}

	gxflash_erasedata(mtd->offset + instr->addr, instr->len);
	instr->state = MTD_ERASE_DONE;

	return 0;
}

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
	     u_char *buf)
{
	gxflash_readdata(mtd->offset + from, buf, len);
	*retlen = len;

	return 0;
}

int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
	      const u_char *buf)
{
	gxflash_pageprogram(mtd->offset + to, (unsigned char*)buf, len);
	*retlen = len;

	return 0;
}

int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	return gxflash_block_isbad(mtd->offset + ofs);
}

int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	return gxflash_block_markbad(mtd->offset + ofs);
}

uint64_t mtd_get_device_size(const struct mtd_info *mtd)
{
	return gxflash_get_info(GX_FLASH_CHIP_SIZE);
}

void put_mtd_device(struct mtd_info *mtd)
{
}

