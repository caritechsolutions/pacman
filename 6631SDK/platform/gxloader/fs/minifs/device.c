#include "minifs.h"

const char minifs_countBitsTable[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

int device_IsBlockBad(minifs_device *dev, int blk)
{
	minifs_PageTags tags;
	int start_page = device_BlockStartPage(dev, blk);

	device_ReadPage(dev, start_page, NULL, &tags, 1);
	if (minifs_CountBits(tags.blockStatus) < 7)
		return 1;

	device_ReadPage(dev, start_page + 1, NULL, &tags, 1);

	if (minifs_CountBits(tags.blockStatus) < 7)
		return 1;

	return 0;
}

int device_WritePage(struct minifs_device *dev, int page, const uint8_t *data, minifs_PageTags *tags)
{
	int x;

	x = device_BlockStartPage(dev, dev->StartBlock);
	if (page < x) {
		T("**>> minifs page %d is not valid\n", page);
		return MINIFS_FAIL;
	}

	return dev->writePage(dev, page, data, tags);
}

int device_ReadPage(struct minifs_device *dev, int page, uint8_t *data, minifs_PageTags *tags, int doErrorCorrection)
{
	int retVal = -1;
	minifs_PageTags localTag;

	if (!tags && data)
		tags = &localTag;

	retVal  = dev->readPage(dev, page, data, tags);

	return retVal;
}

int device_EraseBlock(struct minifs_device *dev, int blockInNAND)
{
	return dev->eraseBlock(dev, blockInNAND);
}

// 检查block 上page的状态
int device_CheckPageBit(minifs_device *dev,int blk,int page)
{
	uint8_t *blkBits = minifs_BlockBits(dev,blk);
	return (blkBits[page/8] &  (1<<(page & 7))) ? 1 :0;
}

int device_StillSomePageBits(minifs_device *dev, int blk)
{
	int i;
	uint8_t *blkBits = minifs_BlockBits(dev,blk);

	for (i = 0; i < dev->pageBitmapStride; i++) {
		if (*blkBits)
			return 1;
		blkBits++;
	}

	return 0;
}

#if 0
static void print_pagetags(minifs_PageTags *tags, int page)
{
	if (tags->chunkId == MINIFS_MAX_CHUNK_ID)
		return ;
	T(".................. page =%d ...............\n", page);
	T("chunkId .............. = %d\n", tags->chunkId);
	T("serialNumber.......... = %d\n", tags->serialNumber);
	T("byteCount............. = %d\n", tags->byteCount);
	T("objectId.............. = %d\n", tags->objectId);
	T("ecc................... = %d\n", tags->ecc);
	T("unusedStuff........... = %d\n", tags->unusedStuff);
	T("pageStatus............ = %d\n", tags->pageStatus);
	T("blockStatus........... = %d\n", tags->blockStatus);
	T("\n");
}
#endif

static int device_Scan(minifs_device *dev)
{
	minifs_PageTags    tags;
	int                blk;
	int                page;
	int                c;
	minifs_BlockState  state;
	minifs_BlockInfo*  bi;
	minifs_Object*     object;
	struct list_head   *i;
	struct list_head   *tmp;

	// 扫描所有的Blocks
	for (blk = dev->StartBlock; blk <= dev->EndBlock; blk++) {
		bi                = minifs_GetBlockInfo(dev, blk);
		if (bi == NULL) {
			T("%s, bi NULL.\n", __func__);
			while(1);
		}
		device_ClearPageBits(dev, blk);
		bi->pagesInUse    = 0;
		bi->softDeletions = 0;
		state             = MINIFS_BLOCK_STATE_SCANNING;

		if (device_IsBlockBad(dev, blk)) {
			state = MINIFS_BLOCK_STATE_DEAD;
			bi->blockState    = MINIFS_BLOCK_STATE_DEAD;
			bi->needsRetiring = 1;
			T("block %d is bad\n",blk);
		}

		// Read each page object the block.
		for (c = 0; c < device_BlockPageCount(dev, blk) && state == MINIFS_BLOCK_STATE_SCANNING; c++) {
			page = device_BlockStartPage(dev, blk) + c;
			device_ReadPage(dev, page, NULL, &tags, 1);
#if 0
			print_pagetags(&tags, page);
#endif
			if (minifs_CountBits(tags.pageStatus) < 6) {
				// A deleted page
				dev->FreePages++;
				//T(" %d %d deleted\n", blk, c);
			}
			else if (tags.objectId == MINIFS_UNUSED_OBJECT_ID) {
				// An unassigned page object the block This means that either the
				// block is empty or this is the one being allocated from

				if (c == 0) {
					// the block is unused
					state = MINIFS_BLOCK_STATE_EMPTY;
					dev->ErasedBlocks++;
				}
				else {
					// this is the block being allocated from
					//T("Allocating from page %d at %d\n", c, blk);
					state = MINIFS_BLOCK_STATE_ALLOCATING;
					dev->allocationBlock = blk;
					dev->allocationPage = c;
				}

				dev->FreePages += (device_BlockPageCount(dev, blk) - c);
			}
			else if (tags.chunkId > 0) { // 该页是某对象的一个数据页，加入到对应的对象
				int endpos;

				// A data page.
				device_SetPageBit(dev, blk, c);
				bi->pagesInUse++;

				object = minifs_FindOrCreateObjectById(dev, NULL, tags.objectId, MINIFS_OBJECT_TYPE_FILE, 0, 0, 0644);
				// PutPageIntoFIle checks for a clash (two data pages with the same pageId).
				minifs_PutPageIntoObject(object, tags.chunkId, page, 1);
				endpos = (tags.chunkId - 1) * dev->DataBytesPerChunk + tags.byteCount;

				if (object->variant.file_variant.scannedSize < endpos)
					object->variant.file_variant.scannedSize = endpos;
			}
			else { // 对象头信息页
				uint8_t pageData[MINIFS_BYTES_PER_CHUNK];
				minifs_ObjectHeader *oh = (minifs_ObjectHeader *)pageData;
				minifs_Object *parent;
				// pageId == 0, so it is an ObjectHeader.
				// Thus, we read object the object header and make the object

				// 正常情况下objectId不会等于1，但是在断电情况下flash
				// 数据可能被破坏，导致某个节点的objectId被修改成了1，
				// 这样会将root节点的数据破坏掉，导致整个文件系统异常
				// 这样做只是一种规避方式
				if (tags.objectId == 1)
					continue;

				device_SetPageBit(dev, blk, c);
				bi->pagesInUse++;
				device_ReadPage(dev, page, pageData, NULL, 1);
				if (oh->type != MINIFS_OBJECT_TYPE_UNKNOWN)
				{
					if (oh->flag_head == 0x5a || oh->flag_end == 0x5a)
						parent = NULL;
					else
						parent = minifs_FindOrCreateObjectById(dev, NULL, oh->parent_obj_id,
								MINIFS_OBJECT_TYPE_DIRECTORY, 0, 0, 0755);
					object = minifs_FindOrCreateObjectById(dev, parent, tags.objectId,
							oh->type, oh->yst_uid, oh->yst_gid, oh->yst_mode);

					memcpy(&object->Header, oh, sizeof(minifs_ObjectHeader));
					object->chunkId  = page; //  Page
					object->sum      = strlen(oh->name);

					if (oh->type != MINIFS_OBJECT_TYPE_DIRECTORY)
						object->variant.file_variant.fileSize = oh->objectSize;
				}
			}
		}

		if (state == MINIFS_BLOCK_STATE_SCANNING) {
			// If we got this far while scanning, then the block is fully allocated.
			state = MINIFS_BLOCK_STATE_FULL;
		}

		bi->blockState = state;
		// Now let's see if it was dirty
		if (bi->pagesInUse == 0 && bi->blockState == MINIFS_BLOCK_STATE_FULL)
			minifs_BlockBecameDirty(dev, blk);
	}

#if 0
	list_for_each(i, &dev->root->variant.dir_variant.children) {
		minifs_Object *obj = list_entry(i, minifs_Object, siblings);
		T("root child: %s\n", obj->Header.name);
	}
#endif

	for (blk = dev->StartBlock; blk <= dev->EndBlock; blk++) {
		if (device_IsBlockBad(dev, blk)) {
			minifs_BadBlockRecycling(dev, blk);
		}
	}

	for (i = dev->scanTempList.next; i != &dev->scanTempList;) {
		tmp = i->next;
		minifs_Object *obj = list_entry(i, minifs_Object, siblings);
		T("----------------------less file :%s\n", obj->Header.name);
		minifs_DeleteObject(obj);
		i = tmp;
	}

	return MINIFS_OK;
}

int device_Init(minifs_device *dev)
{
	unsigned x;
	int      bits;
	int      extraBits;
	int      nBlocks;

	// Do we need to add an offset to use block 0?
	if (dev->ReservedBlocks <  0
			|| dev->StartBlock <= 0
			|| dev->EndBlock   <= 0
			|| dev->EndBlock   <= (dev->StartBlock + dev->ReservedBlocks))
	{
		return MINIFS_FAIL;
	}

	dev->currentDirtyChecker  = 0;

	x = dev->page_count;
	for (bits = extraBits = 0; x > 1; bits++) {
		if (x & 1)
			extraBits++;
		x >>= 1;
	}

	if (extraBits > 0)
		bits++;

	// Level0 Tnodes are 16 bits, so if the bitwidth of the page range we're using
	// is greater than 16 we need to figure out page shift and pageGroupSize
	if (bits <= 16)
		dev->pageGroupBits = 0;
	else
		dev->pageGroupBits = bits - 16;

	dev->pageGroupSize = 1 << dev->pageGroupBits;

	if (dev->PagesMinBlock < dev->pageGroupSize) {
		// We have a problem because the soft delete won't work if
		// the page group size > pages per block.
		// This can be remedied by using larger "virtual blocks".

		return MINIFS_FAIL;
	}

	// More device initialisation
	dev->ErasedBlocks = 0;
	dev->freeTnodes   = NULL;
	dev->DataBytesPerChunk = MINIFS_BYTES_PER_CHUNK;

	nBlocks = dev->EndBlock - dev->StartBlock + 1;
	dev->allocationBlock  = -1; // force it to get a new one

	dev->pageBitmapStride = (dev->PagesMaxBlock + 7) / 8;
	dev->pageBits         = YMALLOC(dev->pageBitmapStride * nBlocks);

	if (dev->pageBits)
		memset(dev->pageBits, 0, dev->pageBitmapStride * nBlocks);

	INIT_LIST_HEAD(&dev->scanTempList);
	dev->root = minifs_CreateNewObject(dev, NULL, -1, MINIFS_OBJECT_TYPE_DIRECTORY, 0, 0, 0775);
	dev->root->Header.type = MINIFS_OBJECT_TYPE_DIRECTORY;
	strcpy(dev->root->Header.name, "");

	// Now scan the flash.
	device_Scan(dev);

	return MINIFS_OK;
}
