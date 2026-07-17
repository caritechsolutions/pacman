/*******************************************************************************
FileName          : $relfile
Author            : zhuzhiguo
Version           : 1.0
Date              : 2008.08.28
Description       : 通用代码
Function List     :

 ********************************************************************************
History:
 ********************************************************************************/
#include "minifs.h"

// 创建新的结点
static int minifs_CreateTnodes(minifs_device *dev, int nTnodes)
{
	int i;
	minifs_Tnode *newTnodes;

	if (nTnodes < 1) return MINIFS_OK;

	newTnodes = YMALLOC(nTnodes * sizeof(minifs_Tnode));

	if (!newTnodes) {
		T("minifs: Could not allocate Tnodes\n");
		return MINIFS_FAIL;
	}

	// Hook them into the free list
	for (i = 0; i < nTnodes - 1; i++)
		newTnodes[i].internal[0] = &newTnodes[i+1];

	newTnodes[nTnodes - 1].internal[0] = dev->freeTnodes;
	dev->freeTnodes = newTnodes;

	//	T("minifs: Tnodes added\n");

	return MINIFS_OK;
}

// GetTnode gets us a clean tnode. Tries to make allocate more if we run out
minifs_Tnode *minifs_GetTnode(minifs_device *dev)
{
	minifs_Tnode *tn = NULL;

	// If there are none left make more
	if (!dev->freeTnodes)
		minifs_CreateTnodes(dev, MINIFS_ALLOCATION_NTNODES);

	if (dev->freeTnodes) {
		tn = dev->freeTnodes;
		dev->freeTnodes = dev->freeTnodes->internal[0];
		memset(tn, 0, sizeof(minifs_Tnode));
	}

	return tn;
}

// FreeTnode frees up a tnode and puts it back on the free list
void minifs_FreeTnode(minifs_device *dev, minifs_Tnode *tn)
{
	if (tn) {
		tn->internal[0] = dev->freeTnodes;
		dev->freeTnodes = tn;
	}
}

static int minifs_FindBlockForAllocation(minifs_device *dev) // 分配一块
{
	int i;

	minifs_BlockInfo *bi;

	if (dev->ErasedBlocks < 1) {
		// Hoosterman we've got a problem.
		// Can't get space to gc
		T("minifs tragedy: no space during gc\n");

		return -1;
	}

	// Find an empty block.
	for (i = dev->StartBlock; i <= dev->EndBlock; i++) {
		dev->allocationBlockFinder++;
		if (dev->allocationBlockFinder < dev->StartBlock || dev->allocationBlockFinder> dev->EndBlock)
			dev->allocationBlockFinder = dev->StartBlock;

		bi = minifs_GetBlockInfo(dev, dev->allocationBlockFinder);

		if (bi->blockState == MINIFS_BLOCK_STATE_EMPTY) {
			bi->blockState = MINIFS_BLOCK_STATE_ALLOCATING; // 将该置为分配状态
			dev->ErasedBlocks--;

			//T("Alloc Block =%d\n", dev->allocationBlockFinder);

			return dev->allocationBlockFinder;
		}
	}

	return -1;
}

int minifs_AllocatePage(minifs_device *dev, int useReserve)
{
	int               retVal;
	minifs_BlockInfo  *bi;

	if (dev->allocationBlock < 0) {
		// Get next block to allocate off
		dev->allocationBlock = minifs_FindBlockForAllocation(dev);
		dev->allocationPage = 0;
	}

	if (!useReserve &&  dev->ErasedBlocks <= dev->ReservedBlocks) {
		T("Not enougth space ErasedBlocks=%d\n", dev->ErasedBlocks);
		// Not enough space to allocate unless we're allowed to use the reserve.
		return -1;
	}

	// Next page please....
	if (dev->allocationBlock >= 0) {
		bi = minifs_GetBlockInfo(dev, dev->allocationBlock);

		// 找到page 编号
		// dev->allocationPage 当前block的page
		retVal = device_BlockStartPage(dev, dev->allocationBlock) + dev->allocationPage;
		bi->pagesInUse++;
		device_SetPageBit(dev, dev->allocationBlock, dev->allocationPage);

		dev->allocationPage++;
		dev->FreePages--;

		// 如果这一个block分配完了
		if (dev->allocationPage >= device_BlockPageCount(dev, dev->allocationBlock)) {
			bi->blockState = MINIFS_BLOCK_STATE_FULL;
			dev->allocationBlock = -1;
		}

		return retVal;
	}
	T("!!!!!!!!! Allocator out !!!!!!!!!!!!!!!!!\n");

	return -1;
}

// 将bad block回收
void minifs_BadBlockRecycling(minifs_device *dev, int blockNo)
{
	int erasedOk = 0;
	minifs_BlockInfo *bi = NULL;

	erasedOk = device_EraseBlock(dev, blockNo);
	if (erasedOk) {
		bi = minifs_GetBlockInfo(dev, blockNo);
		if (bi == NULL) {
			T("%s %d, bi NULL.\n", __func__, __LINE__);
			while(1);
		}
		dev->ErasedBlocks++;
		bi->pagesInUse    = 0;
		bi->softDeletions = 0;
		bi->needsRetiring = 0;
		bi->blockState = MINIFS_BLOCK_STATE_EMPTY;
		device_ClearPageBits(dev, blockNo);
	}
}

// 将block置为脏块
void minifs_BlockBecameDirty(minifs_device *dev, int blockNo)
{
	minifs_BlockInfo *bi = minifs_GetBlockInfo(dev,blockNo);

	int erasedOk = 0;

	// If the block is still healthy erase it and mark as clean.
	// If the block has had a data failure, then retire it.
	bi->blockState = MINIFS_BLOCK_STATE_DIRTY;

	if (!bi->needsRetiring) {
		erasedOk = device_EraseBlock(dev, blockNo);
		if (!erasedOk)
			T("**>> Erasure failed %d\n",blockNo);
	}

	if ( erasedOk ) {
		// Clean it up...
		bi->blockState = MINIFS_BLOCK_STATE_EMPTY;
		dev->ErasedBlocks++;
		bi->pagesInUse    = 0;
		bi->softDeletions = 0;
		device_ClearPageBits(dev, blockNo);

		//T("Erased block %d\n", blockNo);
	}
	else {
		// Ding the blockStatus in the first two pages of the block.
		minifs_PageTags  tags;
		minifs_BlockInfo*  bi;

		memset(&tags, 0xff, sizeof(minifs_PageTags));

		tags.blockStatus = 0;

		device_WritePage(dev, device_BlockStartPage(dev, blockNo) + 0, NULL, &tags);
		device_WritePage(dev, device_BlockStartPage(dev, blockNo) + 1, NULL, &tags);

		bi = minifs_GetBlockInfo(dev, blockNo);
		bi->blockState = MINIFS_BLOCK_STATE_DEAD;

		T("**>> Block %d retired\n", blockNo);
	}
}

// 使用二分查找法，快速找到逻辑所在的块上面
int minifs_FindBlock(minifs_device *dev, int pageId)
{
	int low = 0, high= dev->block_count;
	static int mid = -1;

#define PAGE_END(blk) device_BlockStartPage(dev, blk) + device_BlockPageCount(dev, blk)

	// 更快速度找到块 （连续页可能会存在上一次找到块上面）
	if (mid != -1) {
		if (pageId >= device_BlockStartPage(dev, mid) && pageId < PAGE_END(mid)) {
			return mid;
		}
	}

	while (low <= high) {
		mid = (low + high) >> 1;
		if (pageId < device_BlockStartPage(dev, mid))
			high = mid - 1;
		else if (pageId >= PAGE_END(mid))
			low = mid + 1;
		else if (pageId >= device_BlockStartPage(dev, mid) && pageId < PAGE_END(mid)) {
			return mid;
		}
	}

	return -1;
}

void minifs_DeletePage(minifs_device *dev ,int pageId, int markNAND)
{
	int                block;
	int                page;
	minifs_PageTags    tags;
	minifs_BlockInfo*  bi;

	if (pageId <= 0) return;

	block = minifs_FindBlock(dev, pageId);
	page  = pageId - device_BlockStartPage(dev, block);

	if (markNAND) {
		memset(&tags, 0xFF, sizeof(minifs_PageTags));

		tags.pageStatus = 0; // To mark it as deleted.

		device_WritePage(dev, pageId, NULL, &tags);
	}

	bi = minifs_GetBlockInfo(dev, block);

	// Pull out of the management area.
	// If the whole block became dirty, this will kick off an erasure.
	if (bi->blockState == MINIFS_BLOCK_STATE_ALLOCATING || bi->blockState == MINIFS_BLOCK_STATE_FULL) {
		dev->FreePages++;
		device_ClearPageBit(dev, block, page);
		bi->pagesInUse--;

		if (bi->pagesInUse == 0 && bi->blockState == MINIFS_BLOCK_STATE_FULL)
			minifs_BlockBecameDirty(dev, block);
		else {

		}
	}
}

