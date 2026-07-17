#include <config.h>
#include <gx_flash_common.h>
#include "flash.h"
#include "minifs_def.h"
#include "minifs_diskif.h"

static int spiflash_ReadPage(minifs_device *dev, int chunk, u8 *data, minifs_PageTags *tags)
{
	int err = 0;
	u32 len = MINIFS_BYTES_PER_CHUNK;
	u32 read_buffer_offset;

	chunk--;
	if (data) {
		len = MINIFS_BYTES_PER_CHUNK;
		read_buffer_offset = chunk * MINIFS_HW_PAGE_SIZE + dev->StartAddr;
		err = gxflash_readdata(read_buffer_offset, (unsigned char *)data, len);

		if (err != 0)
			return MINIFS_FAIL;
	}

	if (tags) {
		len = sizeof(minifs_PageTags);
		read_buffer_offset = chunk * MINIFS_HW_PAGE_SIZE + MINIFS_BYTES_PER_CHUNK + dev->StartAddr;
		err = gxflash_readdata(read_buffer_offset, (unsigned char *)tags, len);

		if (err != 0)
			return MINIFS_FAIL;
	}

	return MINIFS_OK;
}

static int spiflash_WritePage(minifs_device *dev, int chunk, const u8 *data, minifs_PageTags *tags)
{
	int err = 0;
	u32 write_buffer_offset, len;

	chunk--;
	if (data) {
		len = MINIFS_BYTES_PER_CHUNK;
		write_buffer_offset = chunk * MINIFS_HW_PAGE_SIZE + dev->StartAddr;
		err = gxflash_pageprogram(write_buffer_offset, (unsigned char *)data, len);

		if (err != 0)
			return MINIFS_FAIL;
	}

	if (tags) {
		len = sizeof(minifs_PageTags);
		write_buffer_offset = chunk * MINIFS_HW_PAGE_SIZE + MINIFS_BYTES_PER_CHUNK + dev->StartAddr;
		err = gxflash_pageprogram(write_buffer_offset, (unsigned char *)tags, len);

		if (err != 0)
			return MINIFS_FAIL;
	}

	return MINIFS_OK;
}

static int spiflash_EraseBlock(minifs_device *dev, int blockNumber)
{
	u32 len = 0;
	u32 offset = 0;
	int start_page = device_BlockStartPage(dev, blockNumber);

	offset = (start_page - 1) * MINIFS_HW_PAGE_SIZE + dev->StartAddr;
	len = (device_BlockPageCount(dev, blockNumber)) * MINIFS_HW_PAGE_SIZE;
	gxflash_erasedata(offset, len);

	return MINIFS_OK;
}

int spiflash_Init(minifs_device *dev)
{
	int i = 0, pageCount = 0;
	int dev_size = 0;

	dev->self = NULL;
	dev->writePage  = spiflash_WritePage;
	dev->readPage   = spiflash_ReadPage;
	dev->eraseBlock = spiflash_EraseBlock;

	dev_size = dev->EndAddr - dev->StartAddr;
	dev->block_count = dev_size / FLASH_BLOCK_SIZE;


	// ³õÊ¼»¯¿éÐÅÏ¢
	dev->blocks = (minifs_Block*)YMALLOC((dev->block_count + 1) * sizeof(minifs_Block));
	if (!dev->blocks)
		return MINIFS_FAIL;
	memset(dev->blocks, 0, (dev->block_count + 1) * sizeof(minifs_Block));

#define INT32_MAX              (2147483647)
	pageCount = 1;
	dev->PagesMaxBlock = 0;
	dev->PagesMinBlock = INT32_MAX;
	for (i=1; i<= dev->block_count; i++) {
		dev->blocks[i].page_count = FLASH_BLOCK_SIZE / MINIFS_HW_PAGE_SIZE;
		dev->blocks[i].start_page = pageCount;

		if (dev->blocks[i].page_count > dev->PagesMaxBlock)
			dev->PagesMaxBlock = dev->blocks[i].page_count;
		if (dev->blocks[i].page_count < dev->PagesMinBlock)
			dev->PagesMinBlock = dev->blocks[i].page_count;
		pageCount += dev->blocks[i].page_count;
	}

	dev->page_count = pageCount;

	if (dev->EndBlock > dev->block_count)
		dev->EndBlock = dev->block_count;

	return MINIFS_OK;
}
