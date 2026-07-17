#include "minifs_def.h"

#define MINIFS_PASSIVE_GC_CHUNKS 2

static int minifs_TagsMatch(const minifs_PageTags *tags, int objectId, int pageInObject)
{
	int pageDeleted = minifs_CountBits(tags->pageStatus) < 7 ? 1 : 0;

	return (tags->chunkId  == pageInObject &&
			tags->objectId == objectId && !pageDeleted) ? 1 : 0;
}

#ifdef USE_ECC
static void minifs_CalcTagsECC(minifs_PageTags *tags)
{
	unsigned char *b = (unsigned char *)tags;
	unsigned      i, j;
	unsigned      ecc = 0;
	unsigned      bit = 0;

	tags->ecc = 0;

	for (i = 0; i < 8; i++) {
		for (j = 1; j &0xff; j<<=1) {
			bit++;
			if (b[i] & j)
				ecc ^= bit;
		}
	}

	tags->ecc = ecc;
}
#endif

static minifs_Object *minifs_find_id(minifs_Object *obj, int objectId)
{
	minifs_Object *found = NULL;
	struct list_head *i;
	minifs_Object *object;

	if (obj->objectId == objectId)
		return obj;

	list_for_each(i, &obj->variant.dir_variant.children) {
		object = list_entry(i, minifs_Object, siblings);
		if (object->objectId == objectId) {
			found = object;
			break;
		}
		if (object->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY) {
			found = minifs_find_id(object, objectId);
			if (found != NULL)
				break;
		}
	}

	return found;
}

minifs_Object *minifs_FindObjectByIdFromRootTree(minifs_device *dev, int objectId)
{
	return minifs_find_id(dev->root, objectId);
}

minifs_Object *minifs_FindObjectByIdFromScanTempList(minifs_device *dev, int objectId, int *top_level)
{
	minifs_Object *obj;
	minifs_Object *theObject;
	struct list_head *i;

	list_for_each(i, &dev->scanTempList) {
		obj = list_entry(i, minifs_Object, siblings);
		if (obj) {
			if (obj->objectId == objectId) {
				theObject = obj;
				*top_level = 1;
				return theObject;
			} else if (obj->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY) {
				theObject = minifs_find_id(obj, objectId);
				if (theObject)
					return theObject;
			}
		}
	}

	return NULL;
}

static int minifs_GenbjectNumber(minifs_device *dev)
{
	uint32_t n = 1;
	int found = 0;
	int top_level = 0;

	if (dev->root == NULL)
		return 1;

	while (!found) {
		found = 1;
		n++;
		if (minifs_FindObjectByIdFromRootTree(dev, n) != NULL)
			found = 0;

		if (found == 0) {
			if (minifs_FindObjectByIdFromScanTempList(dev, n, &top_level) != NULL)
				found = 0;
		}
	}

	return n;
}

minifs_Object *minifs_CreateNewObject(minifs_device *dev, minifs_Object *parent, int objectId,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode)
{
	minifs_Object *theObject;

	theObject = YMALLOC(sizeof(minifs_Object));
	if (theObject == NULL)
		return NULL;

	memset(theObject, 0, sizeof(minifs_Object));

	if (objectId < 0)
		objectId = minifs_GenbjectNumber(dev);

	theObject->fake            = 0;
	theObject->device          = dev;
	theObject->chunkId         = -1;
	theObject->objectId        = objectId;
	theObject->sum             = 0;
	theObject->Cache.chunkId   = -1;
	theObject->parent          = parent;
	theObject->Header.yst_gid  = gid;
	theObject->Header.yst_uid  = uid;
	theObject->Header.yst_mode = mode;
	theObject->Header.type     = type;
	INIT_LIST_HEAD(&theObject->siblings);

	switch (type) {
		case MINIFS_OBJECT_TYPE_DIRECTORY:
			INIT_LIST_HEAD(&theObject->variant.dir_variant.children);
			break;
		case MINIFS_OBJECT_TYPE_FILE:
			theObject->variant.file_variant.fileSize    = 0;
			theObject->variant.file_variant.scannedSize = 0;
			theObject->variant.file_variant.topLevel    = 0;
			theObject->variant.file_variant.topNode     = minifs_GetTnode(dev);
			break;
		default:
			break;
	}

	if (parent) {
		theObject->Header.parent_obj_id = parent->objectId;
		list_add(&theObject->siblings, &parent->variant.dir_variant.children);
	}

	return theObject;
}

void minifs_FreeObject(minifs_Object *object)
{
	list_del(&object->siblings);

	YFREE((void **)&object);
}

int minifs_Traversal(minifs_device *dev, minifs_Object *parent, int (*process)(minifs_Object *obj))
{
	struct list_head*  i;
	minifs_Object *object;
	int count = 0;

	if (parent->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY) {
		list_for_each(i, &parent->variant.dir_variant.children) {
			object = list_entry(i, minifs_Object, siblings);

			if (process)
				process(object);
			if (object->Header.type ==  MINIFS_OBJECT_TYPE_DIRECTORY) {
				count += minifs_Traversal(dev, object, process);
			}
			count++;
		}
	}

	return count;
}

minifs_Object *minifs_FindObjectByName(minifs_device *dev, const char *name)
{
	struct list_head*  i;
	minifs_Object *parent, *object, *thisp = NULL;
	char *p, *pname, *newp;
	int len;

	if (strcmp(name, "") == 0 || strcmp(name, "/") == 0)
		return dev->root;

	newp = YSTRDUP(name);
	pname = newp;
	parent = dev->root;
	thisp = dev->root;
	if (strncmp(pname, "/", 1) == 0)
		pname++;

	while (1) {
		p = strchr(pname, '/');
		if (p)
			len = p - pname;
		else
			len = strlen(pname);
		thisp = NULL;
		list_for_each(i, &parent->variant.dir_variant.children) {
			object = list_entry(i, minifs_Object, siblings);
			if (object->sum != len)
				continue;

			if (strncmp(object->Header.name, pname, len) == 0) {
				thisp = object;
				parent = object;
				break;
			}
		}
		if (p == NULL || thisp == NULL)
			break;
		pname = p + 1;
	}
	YFREE((void **)&newp);

	return thisp;
}

minifs_Object *minifs_CreateNewObjectByName(minifs_device *dev, const char *name,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode,
		minifs_Object *equiv_obj, const char *alias)
{
	char *pname = YSTRDUP(name);
	char *p = strrchr(pname, '/');
	minifs_Object *parent, *obj = NULL;

	if (p) {
		*p = '\0';
		if (strlen(p+1) > MINIFS_MAX_NAME_LENGTH) {
			YFREE((void **)&pname);
			return NULL;
		}
		parent = minifs_FindObjectByName(dev, (const char*)pname);
		if (parent) {
			obj = minifs_CreateNewObject(dev, parent, -1, type, uid, gid, mode);
			if (type == MINIFS_OBJECT_TYPE_SYMLINK && alias)
				strncpy(obj->Header.alias, alias, MINIFS_MAX_ALIAS_LENGTH - 1);
			else if (type == MINIFS_OBJECT_TYPE_HARDLINK && equiv_obj != NULL)
				obj->Header.equiv_id = equiv_obj->objectId;

			obj->dirty = 1;
			obj->Header.type = type;
			strcpy(obj->Header.name, p + 1);
			minifs_UpdateObjectHeader(obj, p + 1, 0);
		}
	}
	YFREE((void **)&pname);

	return obj;
}

minifs_Object *minifs_FindOrCreateObjectById(minifs_device *dev, minifs_Object *parent, int objectId,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode)
{
	minifs_Object *theObject = NULL;
	int isTempObject = 0;
	int top_level = 0;

	if (objectId > 0) {
		// 从目录树找到对象
		theObject = minifs_FindObjectByIdFromRootTree(dev, objectId);

		// 目录中没有找到，则从扫描临时表中找
		if (theObject == NULL) {
			theObject = minifs_FindObjectByIdFromScanTempList(dev, objectId, &top_level);
			if (theObject)
				isTempObject = 1;
		}
	}

	// 如果已找到
	if (theObject != NULL) {
		if (uid < 0xFFFFFFFF)
			theObject->Header.yst_uid = uid;
		if (gid < 0xFFFFFFFF)
			theObject->Header.yst_gid = gid;
		if (mode !=0)
		theObject->Header.yst_mode = mode;

		// 如果是从临时扫描表中找到，则将其移出临时扫描表，加入到目录中
		if (parent && isTempObject && top_level) {
			list_del(&theObject->siblings);
			theObject->parent = parent;
			list_add(&theObject->siblings, &parent->variant.dir_variant.children);
		}
	}
	else {
		// 新建对象，如果没有父对象，则入临时扫描表中
		theObject = minifs_CreateNewObject(dev, parent, objectId, type, uid, gid, mode);
		if (parent == NULL)
			list_add(&theObject->siblings, &dev->scanTempList);
	}

	return theObject;
}

static minifs_Tnode *minifs_FindLevel0Tnode(minifs_device *dev, minifs_Object *object, uint32_t pageId)
{
	minifs_Tnode *tn = object->variant.file_variant.topNode;
	uint32_t i;
	int requiredTallness = 0;
	int level = object->variant.file_variant.topLevel;

	// Check sane level and page Id
	if (level < 0 || level > MINIFS_TNODES_MAX_LEVEL || pageId > MINIFS_MAX_CHUNK_ID)
		return NULL;

	// First check we're tall enough (ie enough variant.file_variant.topLevel)
	i = pageId >> (MINIFS_TNODES_LEVEL0_BITS); //  = 4

	while (i) {
		i >>= MINIFS_TNODES_INTERNAL_BITS;
		requiredTallness++;
	}

	if (requiredTallness > object->variant.file_variant.topLevel) {
		// Not tall enough, so we can't find it, return NULL.
		return NULL;
	}

	// Traverse down to level 0
	while (level > 0 && tn) {
		tn = tn->internal[(pageId >>(MINIFS_TNODES_LEVEL0_BITS +
					(level-1) * MINIFS_TNODES_INTERNAL_BITS)) & MINIFS_TNODES_INTERNAL_MASK];
		level--;
	}

	return tn;
}

static minifs_Tnode *minifs_AddOrFindLevel0Tnode(minifs_device *dev, minifs_Object *object, uint32_t chunkId)
{
	int requiredTallness;
	int i;
	int l;
	minifs_Tnode *tn;
	uint32_t x;

	/* Check sane level and page Id */
	if (object->variant.file_variant.topLevel < 0 || object->variant.file_variant.topLevel > MINIFS_TNODES_MAX_LEVEL)
		return NULL;

	if (chunkId > MINIFS_MAX_CHUNK_ID)
		return NULL;

	/* First check we're tall enough (ie enough variant.file_variant.topLevel) */

	x = chunkId >> MINIFS_TNODES_LEVEL0_BITS;
	requiredTallness = 0;
	while (x) {
		x >>= MINIFS_TNODES_INTERNAL_BITS;
		requiredTallness++;
	}

	if (requiredTallness > object->variant.file_variant.topLevel) {
		/* Not tall enough, gotta make the tree taller */
		for (i = object->variant.file_variant.topLevel; i < requiredTallness; i++) {

			tn = minifs_GetTnode(dev);

			if (tn) {
				tn->internal[0] = object->variant.file_variant.topNode;
				object->variant.file_variant.topNode = tn;
				object->variant.file_variant.topLevel++;
			}
			else {
				T("minifs: no more tnodes\n");
				return NULL;
			}
		}
	}

	/* Traverse down to level 0, adding anything we need */
	l = object->variant.file_variant.topLevel;
	tn = object->variant.file_variant.topNode;

	while (l > 0 && tn) {
		x = (chunkId >> (MINIFS_TNODES_LEVEL0_BITS + (l - 1) * MINIFS_TNODES_INTERNAL_BITS)) & MINIFS_TNODES_INTERNAL_MASK;

		if (!tn->internal[x]) {
			/* Add missing non-level-zero tnode */
			tn->internal[x] = minifs_GetTnode(dev);
			if (!tn->internal[x])
				return NULL;
		}

		tn = tn->internal[x];
		l--;
	}

	return tn;
}

int minifs_FindPageInFile(minifs_Object *object, int chunk, minifs_PageTags *tags)
{
	int             thePage = -1;
	int             i;
	int             found = 0;
	minifs_Tnode*   tn = NULL;
	minifs_PageTags localTags;
	minifs_device*  dev = object->device;

	if (!tags)
		tags = &localTags;

	tn = minifs_FindLevel0Tnode(dev, object, chunk);
	if (tn)  {
		thePage = tn->level0[chunk & MINIFS_TNODES_LEVEL0_MASK] << dev->pageGroupBits;

		// Now we need to do the shifting etc and search for it
		for (i = 0,found = 0; thePage && i < dev->pageGroupSize && !found; i++) {
			device_ReadPage(dev, thePage, NULL, tags, 1);

			if (minifs_TagsMatch(tags, object->objectId, chunk))
				found = 1;
			else
				thePage++;
		}
	}

	return found ? thePage : -1;
}

int minifs_PutPageIntoObject(minifs_Object *object, int chunk, int pageInDevice, int inScan)
{
	minifs_Tnode*   tn;
	minifs_device*  dev = object->device;
	int             existingPage;
	minifs_PageTags existingTags;
	minifs_PageTags newTags;
	unsigned        existingSerial, newSerial;

	tn = minifs_AddOrFindLevel0Tnode(dev, object, chunk);
	if (!tn)
		return MINIFS_FAIL;

	existingPage = tn->level0[chunk & MINIFS_TNODES_LEVEL0_MASK];

	if (inScan && existingPage != 0) {
		device_ReadPage(dev, pageInDevice, NULL, &newTags, 1);

		existingPage = minifs_FindPageInFile(object, chunk, &existingTags);

		if (existingPage <= 0)
			T("minifs tragedy: existing page < 0 object scan\n");

		newSerial = newTags.serialNumber;
		existingSerial = existingTags.serialNumber;

		if ( existingPage <= 0 || ((existingSerial + 1) & 3) == newSerial) {
			/* Forward scanning.
			 * Use new
			 * Delete the old one and drop through to update the tnode
			 */
			minifs_DeletePage(dev, existingPage, 1);
		}
		else {
			/* Backward scanning or we want to use the existing one
			 * Delete the new one and return early so that the tnode isn't changed
			 */
			minifs_DeletePage(dev, pageInDevice, 1);
			return MINIFS_OK;
		}
	}

	if (existingPage == 0)
		object->dataChunks++;

	tn->level0[chunk & MINIFS_TNODES_LEVEL0_MASK] = (pageInDevice >> dev->pageGroupBits);

	return MINIFS_OK;
}

int minifs_ReadChunkFromObject(minifs_Object *object, int chunk, uint8_t *buffer)
{
	int pageInDevice = minifs_FindPageInFile(object, chunk, NULL);

	if (pageInDevice >= 0)
		return device_ReadPage(object->device, pageInDevice, buffer, NULL, 1);
	else
		memset(buffer, 0, object->device->DataBytesPerChunk);

	return 0;
}

int minifs_ShowPage(minifs_Object *object)
{
	int i = 1;
	int pageInDevice, blk, old = -1;

	while (1) {
		pageInDevice = minifs_FindPageInFile(object, i, NULL);
		blk = minifs_FindBlock(object->device, pageInDevice);

		if (old != -1 && old != blk)
			T("\n");
		old = blk;
		if (pageInDevice >= 0)
			T("%3d -> %3d/%d\t", i, pageInDevice, blk);
		else
			break;

		i++;
	}

	return 0;
}

static int minifs_DoGenericObjectDeletion(minifs_Object *object)
{
	minifs_DeletePage(object->device, object->chunkId, 1);
	object->chunkId = -1;
	minifs_FreeObject(object);

	return MINIFS_OK;
}

static int minifs_WriteNewPageToDevice(minifs_device *dev, const uint8_t *data, minifs_PageTags *tags, int useReserve)
{
	int  page;
	int  writeOk = 1;

	do {
		page = minifs_AllocatePage(dev, useReserve);

		if (page >= 0) {
			writeOk = device_WritePage(dev, page, data, tags);
		}
	}
	while (page >= 0 && ! writeOk);

	return page;
}

static int minifs_GarbageCollectBlock(minifs_device *dev, int block)
{
	int              oldPage;
	int              newPage;
	int              pageInBlock;
	int              markNAND;
	int              page_count;
	minifs_PageTags  tags;
	uint8_t          buffer[MINIFS_BYTES_PER_CHUNK];
	minifs_Object*   object;

	oldPage = device_BlockStartPage(dev, block);
	page_count = device_BlockPageCount(dev, block);

	for (pageInBlock = 0; pageInBlock < page_count && device_StillSomePageBits(dev, block);
			pageInBlock++, oldPage++ )
	{
		if (device_CheckPageBit(dev,block,pageInBlock)) {
			markNAND = 1;

			device_ReadPage(dev, oldPage, buffer, &tags, 1);
			object = minifs_FindObjectByIdFromRootTree(dev, tags.objectId);

			if (object && object->deleted && tags.chunkId != 0) {
				object->dataChunks--;

				if (object->dataChunks <= 0) {
					minifs_FreeTnode(object->device, object->variant.file_variant.topNode);
					object->variant.file_variant.topNode = NULL;
					T("minifs: About to finally delete object %d\n", object->objectId);
					minifs_DoGenericObjectDeletion(object);
				}
				markNAND = 0;
			}
			else if ( 0 /* Todo object && object->deleted && object->ndataChunks == 0 */) {
				// Deleted object header with no data pages.
				// Can be discarded and the file deleted.
				object->chunkId = 0;
				minifs_FreeTnode(object->device, object->variant.file_variant.topNode);
				object->variant.file_variant.topNode = NULL;
				minifs_DoGenericObjectDeletion(object);
			}
			else if (object) {
				// It's either a data page object a live file or
				// an ObjectHeader, so we're interested in it.
				// NB Need to keep the ObjectHeaders of deleted files
				// until the whole file has been deleted off
				tags.serialNumber++;
				newPage = minifs_WriteNewPageToDevice(dev, buffer, &tags, 1);

				if (newPage < 0)
					return MINIFS_FAIL;

				if (tags.chunkId == 0) {
					object->chunkId = newPage;
					object->serial = tags.serialNumber;
				}
				else
					minifs_PutPageIntoObject(object, tags.chunkId, newPage, 0);
			}
			minifs_DeletePage(dev, oldPage, markNAND);
		}
	}

	return MINIFS_OK;
}

static int minifs_FindDirtiestBlock(minifs_device *dev, int aggressive)
{
	int b = dev->currentDirtyChecker;

	int              i;
	int              iterations;
	int              dirtiest = -1;
	int              pagesInUse;
	minifs_BlockInfo* bi;

	// If we're doing aggressive GC then we are happy to take a less-dirty block,
	// and search further.

	pagesInUse = (aggressive)? dev->PagesMaxBlock : MINIFS_PASSIVE_GC_CHUNKS + 1;
	if (aggressive)
		iterations = dev->EndBlock - dev->StartBlock + 1;
	else {
		iterations = (dev->EndBlock - dev->StartBlock + 1) / 16;
		if (iterations > 200)
			iterations = 200;
	}

	for (i = 0; i <= iterations && pagesInUse > 0 ; i++) {
		b++;
		if ( b < dev->StartBlock || b > dev->EndBlock)
			b =  dev->StartBlock;

		if (b < dev->StartBlock || b > dev->EndBlock) {
			T("**>> Block %d is not valid\n", b);
			//YBUG();
		}

		bi = minifs_GetBlockInfo(dev, b);

		if (bi->blockState == MINIFS_BLOCK_STATE_FULL &&
				(bi->pagesInUse - bi->softDeletions )< pagesInUse)
		{
			dirtiest = b;
			pagesInUse = (bi->pagesInUse - bi->softDeletions);
		}
	}

	dev->currentDirtyChecker = b;

	return dirtiest;
}

static int minifs_CheckGarbageCollection(minifs_device *dev)
{
	int block;
	int aggressive=0;

	if (dev->ErasedBlocks <= (dev->ReservedBlocks + 1))
		aggressive = 1;

	block = minifs_FindDirtiestBlock(dev, aggressive);

	if (block >= 0) {
		//T("minifs: GC erasedBlocks %d aggressive %d\n", dev->ErasedBlocks, aggressive);
		return minifs_GarbageCollectBlock(dev, block);
	}

	return aggressive ? MINIFS_FAIL : MINIFS_OK;
}

void minifs_FlushObjectPageCache(minifs_Object *object)
{
	if (object->Cache.Bytes > 0) {
		minifs_WriteChunkToObject(object,
				object->Cache.chunkId,
				object->Cache.data,
				object->Cache.Bytes, 1);
	}
}

int minifs_WriteChunkToObject(minifs_Object *object, int chunk,
		const uint8_t *buffer, int nBytes, int useReserve)
{
	int              prevPageId;
	int              newPageId;
	minifs_PageTags  prevTags;
	minifs_PageTags  newTags;

	minifs_device *dev = object->device;

	minifs_CheckGarbageCollection(dev);

	// Get the previous page at this location in the file if it exists
	prevPageId  = minifs_FindPageInFile(object, chunk, &prevTags);

	// Set up new tags
	memset(&newTags, 0xFF, sizeof(minifs_PageTags));
	newTags.chunkId      = chunk;
	newTags.objectId     = object->objectId;
	newTags.serialNumber = (prevPageId >= 0) ? prevTags.serialNumber + 1 : 1;
	newTags.byteCount    = nBytes;

#ifdef USE_ECC
	minifs_CalcTagsECC(&newTags);
#endif
	newPageId = minifs_WriteNewPageToDevice(dev, buffer, &newTags, useReserve);

	if (newPageId >= 0) {
		minifs_PutPageIntoObject(object, chunk, newPageId, 0);
		if (prevPageId >= 0)
			minifs_DeletePage(dev, prevPageId, 1);
	}
	return newPageId;
}

int minifs_UpdateObjectHeader(minifs_Object *obj, const char *name, int force)
{
	int                  prevChunkId;
	int                  newChunkId;
	minifs_PageTags      newTags;
	uint8_t              bufferNew[MINIFS_BYTES_PER_CHUNK];

	minifs_ObjectHeader* oh  = (minifs_ObjectHeader *)bufferNew;

	minifs_device *dev = obj->device;
	if (!obj->fake || force) {
		minifs_CheckGarbageCollection(dev);
		memset(bufferNew, 0xFF, MINIFS_BYTES_PER_CHUNK);
		prevChunkId = obj->chunkId;

		obj->Header.objectSize = obj->variant.file_variant.fileSize;
		memcpy(oh, &obj->Header, sizeof(minifs_ObjectHeader));
		if (name && *name) {
			obj->sum = strlen(name);
			strncpy(obj->Header.name, name, obj->sum);
		}

		memset (oh->name, 0, MINIFS_MAX_NAME_LENGTH + 1);
		strncpy(oh->name, obj->Header.name, obj->sum);
		oh->objectSize    = obj->variant.file_variant.fileSize;
		oh->parent_obj_id = obj->Header.parent_obj_id;
		oh->yst_mode      = obj->Header.yst_mode;
		oh->type          = obj->Header.type;
		oh->rawFileSize   = obj->Header.rawFileSize;

		if (force == -1)
			oh->flag_head = oh->flag_end = 0x5a;
		else
			oh->flag_head = oh->flag_end = 0xff;

		// Tags
		obj->serial++;
		memset(&newTags, 0xFF, sizeof(minifs_PageTags));
		newTags.chunkId      = 0;
		newTags.objectId     = obj->objectId;
		newTags.serialNumber = obj->serial;
		newTags.byteCount    = 0x3FF;
#ifdef USE_ECC
		minifs_CalcTagsECC(&newTags);
#endif
		// Create new chunk in NAND
		newChunkId = minifs_WriteNewPageToDevice(dev, bufferNew, &newTags, (prevChunkId >= 0) ? 1 : 0 );
		if (newChunkId >= 0) {
			obj->chunkId = newChunkId;
			if (prevChunkId >= 0)
				minifs_DeletePage(dev, prevChunkId, 1);

			obj->dirty = 0;
		}

		return newChunkId;
	}

	return 0;
}

static int minifs_DeleteWorker(minifs_Object *obj, minifs_Tnode *tn, uint32_t level, int chunkOffset,int *limit)
{
	int i;
	int chunkInInode;
	int theChunk;
	minifs_PageTags tags;
	int found;
	int allDone = 1;

	if (tn) {
		if (level > 0) {
			for (i = MINIFS_NTNODES_INTERNAL -1; allDone && i >= 0; i--) {
				if (tn->internal[i]) {
					if (limit && (*limit) < 0) {
						allDone = 0;
					}
					else {
						allDone = minifs_DeleteWorker(obj, tn->internal[i], level - 1,
								(chunkOffset << MINIFS_TNODES_INTERNAL_BITS ) + i, limit);
					}
					if (allDone) {
						minifs_FreeTnode(obj->device, tn->internal[i]);
						tn->internal[i] = NULL;
					}
				}

			}
			return (allDone) ? 1 : 0;
		}
		else if (level == 0) {
			int hitLimit = 0;

			for (i = MINIFS_NTNODES_LEVEL0 -1; i >= 0 && !hitLimit; i--) {
				if (tn->level0[i]) {
					int j;

					chunkInInode = (chunkOffset << MINIFS_TNODES_LEVEL0_BITS ) + i;

					theChunk =  tn->level0[i] << obj->device->pageGroupBits;

					// Now we need to search for it
					for (j = 0, found = 0; theChunk && j < obj->device->pageGroupSize && !found; j++) {
						device_ReadPage(obj->device, theChunk, NULL, &tags, 1);

						if (minifs_TagsMatch(&tags, obj->objectId, chunkInInode)) {
							found = 1;
						}
						else {
							theChunk++;
						}
					}

					if (found) {
						minifs_DeletePage(obj->device, theChunk, 1);
						obj->dataChunks--;
						if (limit) {
							*limit = *limit-1;
							if (limit <= 0)
							{
								hitLimit = 1;
							}
						}
					}

					tn->level0[i] = 0;
				}
			}
			return (i < 0) ? 1 : 0;
		}
	}

	return 1;
}

#if 0
static int minifs_SoftDeleteWorker(minifs_Object *obj, minifs_Tnode *tn, uint32_t level, int chunkOffset)
{
	int              i;
	int              theChunk, blk;
	minifs_BlockInfo *theBlock;
	int              allDone = 1;


	if (tn == NULL)
		return 1;
	if (level > 0) {
		for (i = MINIFS_NTNODES_INTERNAL -1; allDone && i >= 0; i--) {
			if (tn->internal[i]) {
				allDone = minifs_SoftDeleteWorker(obj, tn->internal[i], level - 1,
						(chunkOffset << MINIFS_TNODES_INTERNAL_BITS ) + i);
				if (allDone) {
					minifs_FreeTnode(obj->device, tn->internal[i]);
					tn->internal[i] = NULL;
				}
			}
		}

		return (allDone) ? 1 : 0;
	}
	else if (level == 0) {
		for (i = MINIFS_NTNODES_LEVEL0 -1; i >=0; i--) {
			if (tn->level0[i]) {
				theChunk = (tn->level0[i] << obj->device->pageGroupBits);
				blk      = minifs_FindBlock(obj->device, theChunk);
				theBlock = minifs_GetBlockInfo(obj->device, blk);
				if (theBlock)
					theBlock->softDeletions++;
				tn->level0[i] = 0;
			}
		}

		return 1;
	}

	return 1;
}
#endif

static int minifs_UnlinkFile(minifs_Object *object)
{
	minifs_DeleteWorker(object, object->variant.file_variant.topNode, object->variant.file_variant.topLevel, 0, NULL);
	minifs_FreeTnode(object->device, object->variant.file_variant.topNode);
	object->variant.file_variant.topNode = NULL;

	return minifs_DoGenericObjectDeletion(object);
}

#if 0
static void minifs_SoftDeleteFile(minifs_Object *obj)
{
	if (obj->deleted && !obj->softDeleted) {
		if (obj->dataChunks <= 0) {
			minifs_FreeTnode(obj->device, obj->variant.file_variant.topNode);
			obj->variant.file_variant.topNode = NULL;
			T("minifs: Deleting empty file %d\n", obj->objectId);
			minifs_DoGenericObjectDeletion(obj);
		}
		else {
			minifs_SoftDeleteWorker(obj, obj->variant.file_variant.topNode, obj->variant.file_variant.topLevel, 0);
			obj->softDeleted = 1;
		}
	}
}
#endif

int minifs_DeleteObject(minifs_Object *object)
{
	int retVal = MINIFS_OK;
	struct list_head *i;
	struct list_head *tmp;

	if (object->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY) {
		minifs_Object *obj;
		list_for_each_safe(i, tmp, &object->variant.dir_variant.children) {
			obj = list_entry(i, minifs_Object, siblings);
			minifs_DeleteObject(obj);
		}
	}

	if (object->dataChunks > 0) {
		// Use soft deletion
		if (!object->unlinked)
			retVal = minifs_UnlinkFile(object);

#if 0
		if (retVal == MINIFS_OK && object->unlinked && !object->deleted) {
			object->deleted = 1;
			minifs_SoftDeleteFile(object);
		}
#endif

		return retVal;
	}
	else {
		// The file has no data chunks so we toss it immediately
		minifs_FreeTnode(object->device, object->variant.file_variant.topNode);
		object->variant.file_variant.topNode = NULL;
		minifs_DoGenericObjectDeletion(object);
	}

	return retVal;
}

char *minifs_GetObjectPath(minifs_Object *object, char *path, size_t size)
{
	char *tmp_path = tmp_path = YMALLOC(PATH_MAX);
	if (tmp_path == NULL) {
		return NULL;
	}

	if (object == NULL)
		return 0;

	strcpy(tmp_path, object->Header.name);
	while(object->parent != NULL) {
		snprintf(path, size, "%s/%s", object->parent->Header.name, tmp_path);
		strcpy(tmp_path, path);
		object = object->parent;
	}

	YFREE((void **)&tmp_path);
	return path;
}

int minifs_FlushObject(minifs_Object *object)
{
	int retVal;

	if (object->dirty) {
		minifs_FlushObjectPageCache(object);
		retVal = (minifs_UpdateObjectHeader(object, NULL, 1) > 0)? MINIFS_OK : MINIFS_FAIL;
	}
	else
		retVal = MINIFS_OK;

	return retVal;
}

int minifs_ReadDataFromObject(minifs_Object *object, uint8_t * buffer, uint32_t offset, int nBytes)
{
	int chunk;
	int start;
	int nToCopy;
	int n = nBytes;
	int nDone = 0;
	minifs_ChunkCache *cache;

	minifs_device *device;

	device = object->device;

	while(n > 0) {
		chunk = offset / device->DataBytesPerChunk + 1; // The first chunk is 1
		start = offset % device->DataBytesPerChunk;

		// OK now check for the curveball where the start and end are object
		// the same chunk.
		if ((start + n) < device->DataBytesPerChunk)
			nToCopy = n;
		else
			nToCopy = device->DataBytesPerChunk - start;

		cache = &object->Cache;

		// If the chunk is already in the cache or it is less than a whole chunk
		// then use the cache (if there is caching)
		// else bypass the cache.
		if ( nToCopy != device->DataBytesPerChunk) {
			if (cache->chunkId != chunk)
				minifs_FlushObjectPageCache(object); // 将Cache里的数据写入

			cache->chunkId = chunk;
			cache->dirty = 0;
			minifs_ReadChunkFromObject(object, chunk, cache->data);
			cache->Bytes = 0;
			cache->dirty = 0;

			memcpy(buffer,&cache->data[start], nToCopy);
		}
		else {
			// A full chunk. Read directly into the supplied buffer.
			minifs_ReadChunkFromObject(object, chunk, buffer);
		}

		n -= nToCopy;
		offset += nToCopy;
		buffer += nToCopy;
		nDone += nToCopy;
	}

	return nDone;
}

int minifs_WriteDataToObject(minifs_Object *object, const uint8_t * buffer, uint32_t offset, int nBytes)
{
	int page;
	int start;
	int nToCopy;
	int n = nBytes;
	int nDone = 0;
	int nToWriteBack;
	int startOfWrite = offset;
	int pageWritten = 0;
	int nBytesRead;
	minifs_device *device = object->device;

	while(n > 0 && pageWritten >= 0) {
		page = offset / device->DataBytesPerChunk + 1;
		start = offset % device->DataBytesPerChunk;

		// OK now check for the curveball where the start and end are object
		// the same page.
		if ((start + n) < device->DataBytesPerChunk) {
			nToCopy = n;

			// Now folks, to calculate how many bytes to write back....
			// If we're overwriting and not writing to then end of file then
			// we need to write back as much as was there before.
			nBytesRead = object->variant.file_variant.fileSize - ((page -1) * device->DataBytesPerChunk);

			if (nBytesRead > device->DataBytesPerChunk)
				nBytesRead = device->DataBytesPerChunk;

			nToWriteBack = (nBytesRead > (start + n)) ? nBytesRead : (start +n);
		}
		else {
			nToCopy = device->DataBytesPerChunk - start;
			nToWriteBack = device->DataBytesPerChunk;
		}

		if (nToCopy != device->DataBytesPerChunk) {
			minifs_ChunkCache *cache = &object->Cache;
			if (cache->chunkId != page) {
				minifs_FlushObjectPageCache(object); // 将Cache里的数据写入
				minifs_ReadChunkFromObject(object, page, cache->data);
			}
			cache->chunkId = page;
			cache->dirty = 1;
			memcpy(&cache->data[start], buffer, nToCopy);
			cache->Bytes = nToWriteBack;
		}
		else
			pageWritten = minifs_WriteChunkToObject(object, page, buffer, device->DataBytesPerChunk, 0);

		if (pageWritten >= 0) {
			n      -= nToCopy;
			offset += nToCopy;
			buffer += nToCopy;
			nDone  += nToCopy;
		}
	}

	// Update file object
	if ((startOfWrite + nDone) > object->variant.file_variant.fileSize)
		object->variant.file_variant.fileSize = (startOfWrite + nDone);

	object->dirty = 1;

	return nDone;
}

