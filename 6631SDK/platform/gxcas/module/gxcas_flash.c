#include <assert.h>
#include <math.h>
#include "gxcore.h"
#include "gxcas_flash.h"
#include "gxcas_misc.h"
#include "gxcas_dbg.h"
#ifdef   ECOS_OS
#include "cyg/fileio/fileio.h"
#endif
#include "autoconf.h"
#ifdef CONFIG_ASYNC_FLASH_WRITE
#include "gxflash_async_api.h"
#endif

#define MAGIC_NUM		(0x88156080)

struct gxcas_nvram
{
	uint32_t    magic;/*ħ???????????????Ƿ?????*/
	char        name[GXCA_MAX_NVRAM];
	char        name_backup[GXCA_MAX_NVRAM];
	handle_t    lock;
	size_t      size;
	uint32_t start_addr;
}nvram_ctrl;

struct {
	uint32_t magic;
	struct partition_info *flash;
	struct partition_info *backup;
	uint32_t backup_addr;
	uint32_t size;
}partition_ctrl;

struct {
	uint32_t magic;
	struct partition_info *flash;
	char   name_backup[GXCA_MAX_NVRAM];
	handle_t    lock;
	size_t      size;
	uint32_t start_addr;
}mixflash_ctrl;

static int32_t _nvram_reinit_flash_data(void);
static int32_t _partition_reinit_flash_data(void);
static int32_t _mix_reinit_flash_data(void);
static int crc_cmp(int32_t crc, uint8_t * buf)
{
	char temp[4] = {0};
	temp[0] = (crc >> 24) & 0xff;
	temp[1] = (crc >> 16) & 0xff;
	temp[2] = (crc >> 8) & 0xff;
	temp[3] = crc & 0xff;

	return memcmp(temp, buf, 4);
}

static int32_t _rebuild_minifs_func(void)
{
#define CASBACK_PARTITION "CASBACK"
#define CASBACK_PATH "/home/casback"
	struct partition *temp = NULL;
	struct partition_info *casback;
	char buf[40] = {0};
	int32_t ret = 0;
	temp = GxCore_PartitionFlashInit();
	if(NULL == temp) {
		CAS_ERR(FLASH, "GxCore_PartitionFlashInit Failed");
		return -1;
	}

	if (!umount(CASBACK_PATH)) {
		CAS_DBG(FLASH, "minifs umount /home/casback/ success\n");
	} else {
		CAS_ERR(FLASH, "minifs umount /home/casback/ fail\n");
		ret = -2;
	}
	casback = GxCore_PartitionGetByName(temp, CASBACK_PARTITION);
	if(casback == NULL){
		CAS_ERR(FLASH, "GxCore_PartitionGetByName Failed!!! ");
		return -1;
	}
	if(0 == GxCore_PartitionErase(casback)){
		CAS_ERR(FLASH, "GxCore_PartitionErase Failed!!! ");
		return -1;
	}

	//mount casback partition minifs
	sprintf(buf, "/dev/flash/0/0x%x,0x%x", casback->start_addr, casback->total_size);
	if (!mount(buf, CASBACK_PATH, "minifs", 0, NULL)) {
		CAS_DBG(FLASH, "minifs mount success\n");
	} else {
		CAS_ERR(FLASH,"minifs mount fail\n");
		ret = -2;
	}
	return ret;
}

static int32_t _nvram_update_crc(const char *name)
{
	handle_t handle = -1;
	char *value = NULL;
	uint32_t ret = 0, crc = 0;
	
	handle = GxCore_Open(name, "r+");
	value = GxCore_Mallocz(nvram_ctrl.size);
	if (value == NULL)
		goto err;
	ret = GxCore_Read(handle, value, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "Read len != nvram_ctrl.size");
		goto err;
	}

	crc = GxCas_Crc32((uint8_t *)value, nvram_ctrl.size - 4);
	value[nvram_ctrl.size - 4] = (crc >> 24) & 0xff;
	value[nvram_ctrl.size - 3] = (crc >> 16) & 0xff;
	value[nvram_ctrl.size - 2] = (crc >> 8) & 0xff;
	value[nvram_ctrl.size - 1] = (crc) & 0xff;
	GxCore_Seek(handle, 0, GX_SEEK_SET);
	ret = GxCore_Write(handle, value, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "Write len != nvram_ctrl.size");
		goto err;
	}

	GxCore_Close(handle);
	GxCore_Free(value);
	return 0;
err:
	if (value)
		GxCore_Free(value);
	if (handle != -1)
		GxCore_Close(handle);
	return -1;
}

static int32_t _nvram_open(const char* name)
{
	uint8_t *value = NULL;
	handle_t handle = -1;
	uint32_t ret = 0;

	if (GxCore_FileExists(name) == GXCORE_MOD_EXIST) {
		CAS_DBG(FLASH,"File is exist!");
		return 0;
	}

	CAS_DBG(FLASH,"File is not exist!Creat & clear it now");
	value = GxCore_Malloc(nvram_ctrl.size);
	if(value == NULL) {
		CAS_ERR(FLASH,"GxCore_Malloc memory failure!");
		goto err;
	}
	memset(value, 0xFF, nvram_ctrl.size);

	handle = GxCore_Open(name, "c");
	GxCore_Seek(handle, 0, GX_SEEK_SET);
	ret = GxCore_Write(handle, value,1, nvram_ctrl.size);
	if(ret != nvram_ctrl.size) {
		CAS_ERR(FLASH,"Write file err! ret size = %d, write size = %d", ret, nvram_ctrl.size);
		goto err;
	}
	GxCore_Close(handle);
	GxCore_Free(value);

	//return _nvram_update_crc(name);
	return 0;
err:
	if (value)
		GxCore_Free(value);
	if (handle != -1)
		GxCore_Close(handle);
	return -1;
}

static int32_t _nvram_check(void)
{
	handle_t handle_original = -1;
	handle_t handle_backup = -1;
	uint32_t ret = 0, original_crc = 0, backup_crc = 0;
	uint8_t *buf = NULL;
	uint8_t crc_buf[4] = {0};

	buf = GxCore_Mallocz(nvram_ctrl.size);
	if (buf == NULL)
		goto err;
	handle_original = GxCore_Open(nvram_ctrl.name, "r+");
	handle_backup = GxCore_Open(nvram_ctrl.name_backup, "r+");
	ret = GxCore_Read(handle_original, buf, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "original read size != nvram_ctrl.size");
		goto err;
	}
	original_crc = GxCas_Crc32(buf, nvram_ctrl.size - 4);
	if (crc_cmp(original_crc, buf + nvram_ctrl.size - 4) != 0) {
		memset(buf, 0, nvram_ctrl.size);
		ret = GxCore_Read(handle_backup, buf, 1, nvram_ctrl.size);
		if (ret != nvram_ctrl.size) {
			CAS_DBG(FLASH, "backup read size != nvram_ctrl.size");
			goto err;
		}
		backup_crc = GxCas_Crc32(buf, nvram_ctrl.size -4);
		//new backup file
		if ((buf[nvram_ctrl.size -4] == 0xff)
				&&(buf[nvram_ctrl.size - 3] == 0xff)
				&&(buf[nvram_ctrl.size - 2] == 0xff)
				&&(buf[nvram_ctrl.size - 1] == 0xff))
			goto out;

		if (crc_cmp(backup_crc, buf + nvram_ctrl.size - 4) != 0){
			CAS_DBG(FLASH, "Backup Crc Error");
			goto reinit;
		}
		GxCore_Seek(handle_original, 0, GX_SEEK_SET);
		ret = GxCore_Write(handle_original, buf, 1, nvram_ctrl.size);
		if (ret != nvram_ctrl.size) {
			CAS_DBG(FLASH, "original write size != nvram_ctrl.size");
			goto err;
		} else
			goto out;
	}

	//recover backup file
	memset(buf, 0, nvram_ctrl.size);
	GxCore_Seek(handle_original, 0, GX_SEEK_SET);
	ret = GxCore_Read(handle_original, buf, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "original read size != nvram_ctrl.size");
		goto err;
	}
	GxCore_Seek(handle_backup, nvram_ctrl.size - 4, GX_SEEK_SET);
	ret = GxCore_Read(handle_backup, crc_buf, 1, 4);
	if (ret != 4) {
		CAS_DBG(FLASH, "backup read crc err");
		goto err;
	}

	if (crc_cmp(original_crc, crc_buf) != 0) {
		GxCore_Seek(handle_backup, 0, GX_SEEK_SET);
		ret = GxCore_Write(handle_backup, buf, 1, nvram_ctrl.size);
		if (ret != nvram_ctrl.size)
			goto err;
	}

out:
	GxCore_Close(handle_original);
	GxCore_Close(handle_backup);
	GxCore_Free(buf);
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (handle_original != -1)
		GxCore_Close(handle_original);
	if (handle_backup != -1)
		GxCore_Close(handle_backup);
	return -1;
reinit:
	if (buf)
		GxCore_Free(buf);
	if (handle_original != -1)
		GxCore_Close(handle_original);
	if (handle_backup != -1)
		GxCore_Close(handle_backup);
	CAS_DBG(FLASH,  "CA And Backup Data all Error!!");
	return -2;
}

static int32_t _nvram_init(GxCas_FlashConfig flash, GxCas_FlashConfig backup)
{
	/*???ƿ???¼??Ҫ?Ĳ???*/
	int32_t ret = 0;
	nvram_ctrl.magic = MAGIC_NUM;
	nvram_ctrl.size = flash.size;
	nvram_ctrl.start_addr = 0x10000;

	if(strlen(flash.name) >= GXCA_MAX_NVRAM || strlen(backup.name) >= GXCA_MAX_NVRAM) {
		CAS_ERR(FLASH,"name string is longer than max length!");
		goto err;
	} else {
		sprintf(nvram_ctrl.name, "%s", flash.name);
		sprintf(nvram_ctrl.name_backup, "%s", backup.name);
	}

	if (_nvram_open(nvram_ctrl.name) < 0)
		goto err;

	if (backup.size) {
		if (_nvram_open(nvram_ctrl.name_backup) < 0)
			goto err;

		ret = _nvram_check();
		if(ret == -2){
			//TODO: when flash and backup all broken reinit
			if(_nvram_reinit_flash_data()<0)
				goto err;
		}else if(ret < 0)
			goto err;
	}

	CAS_DBG(FLASH,"File name: %s, Backup File name: %s, open size: %d",nvram_ctrl.name, nvram_ctrl.name_backup, nvram_ctrl.size);

	if(0 != GxCore_MutexCreate(&nvram_ctrl.lock)) {
		CAS_ERR(FLASH,"Creat mutex failure!");
		goto err;
	}

	return 0;
err:
	return -1;
}

static size_t _nvram_read(uint32_t offset, uint8_t *buf, size_t len)
{
	handle_t handle;
	size_t   size = 0;
	CAS_DBG(FLASH,"Call.%s",__FUNCTION__);

	if((buf == NULL) || (offset + len > nvram_ctrl.size)){
		CAS_ERR(FLASH,"buff is NULL");
		return -1;
	}

	GxCore_MutexLock(nvram_ctrl.lock);
	handle = GxCore_Open((char*)nvram_ctrl.name, "r");
	GxCore_Seek(handle, (long)offset, GX_SEEK_SET);
	size = GxCore_Read(handle, buf, 1,len);
	if(size != len) {
		CAS_ERR(FLASH,"Read file err! ret size = %d, read size = %d",size,len);
		size = -1;
	}
	GxCore_Close(handle);
	GxCore_MutexUnlock(nvram_ctrl.lock);

	return size;
}

static size_t _nvram_write(const char* name, uint32_t offset, const uint8_t *buf, size_t len)
{
	handle_t handle;
	size_t   size = 0;

	if((buf == NULL) || (offset + len > nvram_ctrl.size) || strlen(name) == 0) {
		CAS_ERR(FLASH,"buff is NULL");
		return -1;
	}

	GxCore_MutexLock(nvram_ctrl.lock);
	handle = GxCore_Open((char*)name, "r+");
	GxCore_Seek(handle, (long)offset, GX_SEEK_SET);
	size = GxCore_Write(handle, (uint8_t*)buf, 1,len);
	if(size != len) {
		CAS_ERR(FLASH,"Write file err! ret size = %d, write size = %d",size,len);
		size = -1;
	}
	GxCore_Close(handle);
	GxCore_MutexUnlock(nvram_ctrl.lock);
	return size;
}

static int32_t _nvram_check_flash_data(const char *name)
{
	handle_t handle = -1;
	uint32_t ret = 0, crc = 0;
	uint8_t *buf = NULL;

	buf = GxCore_Mallocz(nvram_ctrl.size);
	if (buf == NULL)
		goto err;
	handle = GxCore_Open(name, "r+");
	ret = GxCore_Read(handle, buf, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "read size != nvram_ctrl.size");
		goto err;
	}
	crc = GxCas_Crc32(buf, nvram_ctrl.size - 4);
	if (crc_cmp(crc, buf + nvram_ctrl.size - 4) != 0) {
		CAS_DBG(FLASH, "Crc Error");
		goto err;
	}

	CAS_DBG(FLASH, "Write Data Success");
	GxCore_Close(handle);
	GxCore_Free(buf);
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (handle != -1)
		GxCore_Close(handle);
	return -1;
}

static int32_t _nvram_copy_bakup_to_ca(void)
{
	handle_t handle_original = -1;
	handle_t handle_backup = -1;
	uint32_t ret = 0, original_crc = 0, backup_crc = 0;
	uint8_t *buf = NULL;

	buf = GxCore_Mallocz(nvram_ctrl.size);
	if (buf == NULL)
		goto err;
	handle_original = GxCore_Open(nvram_ctrl.name, "r+");
	handle_backup = GxCore_Open(nvram_ctrl.name_backup, "r+");
	ret = GxCore_Read(handle_original, buf, 1, nvram_ctrl.size);
	if (ret != nvram_ctrl.size) {
		CAS_DBG(FLASH, "original read size != nvram_ctrl.size");
		goto err;
	}
	original_crc = GxCas_Crc32(buf, nvram_ctrl.size - 4);
	if (crc_cmp(original_crc, buf + nvram_ctrl.size - 4) != 0) {
		memset(buf, 0, nvram_ctrl.size);
		ret = GxCore_Read(handle_backup, buf, 1, nvram_ctrl.size);
		if (ret != nvram_ctrl.size) {
			CAS_DBG(FLASH, "backup read size != nvram_ctrl.size");
			goto err;
		}
		backup_crc = GxCas_Crc32(buf, nvram_ctrl.size -4);

		if ((buf[nvram_ctrl.size - 4] == 0xff)
				&&(buf[nvram_ctrl.size - 3] == 0xff)
				&&(buf[nvram_ctrl.size - 2] == 0xff)
				&&(buf[nvram_ctrl.size - 1] == 0xff)){
			CAS_DBG(FLASH, "backup partition has not been written");
			goto out;//if backup partition has not been written return success
		}
		//new backup file
		if (crc_cmp(backup_crc, buf + nvram_ctrl.size - 4) != 0){
			CAS_DBG(FLASH, "Backup Crc Error");
			goto err;
		}
		GxCore_Seek(handle_original, 0, GX_SEEK_SET);
		ret = GxCore_Write(handle_original, buf, 1, nvram_ctrl.size);
		if (ret != nvram_ctrl.size) {
			CAS_DBG(FLASH, "original write size != nvram_ctrl.size");
			goto err;
		} else
			goto out;
	}

out:
	GxCore_Close(handle_original);
	GxCore_Close(handle_backup);
	GxCore_Free(buf);
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (handle_original != -1)
		GxCore_Close(handle_original);
	if (handle_backup != -1)
		GxCore_Close(handle_backup);
	return -1;
}

static int32_t _nvram_reinit_flash_data(void)
{
	CAS_ERR(FLASH,"------Delete and rebuild--- \n");
	GxCore_FileDelete(nvram_ctrl.name);
	if (_nvram_open(nvram_ctrl.name) < 0)
		goto err;
	if (strlen(nvram_ctrl.name_backup) != 0) {
		GxCore_FileDelete(nvram_ctrl.name_backup);
		if (_nvram_open(nvram_ctrl.name_backup) < 0)
			goto err;
	}
	return 0;
err:
	return -1;
}

static int32_t _partition_check_crc()
{
	uint8_t* buf = NULL;
	struct partition_info flash_section, backup_sectioin;
	uint32_t ret = 0, i = 0, block_count = 0,  crc_result = 0, backup_crc_result = 0, offset = 0, backup_offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	flash_section.start_addr = partition_ctrl.flash->start_addr;
	flash_section.total_size = partition_ctrl.flash->total_size;
	flash_section.mtd_id = partition_ctrl.flash->mtd_id;
	flash_section.partition_size = partition_ctrl.flash->partition_size;
	block_count = partition_ctrl.size /(GXCAS_BLOCK_SIZE);
	for (i = 0; i < block_count; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		backup_offset = offset + partition_ctrl.backup_addr;
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = GxCore_PartitionRead(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "original Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);
		cmp_value = crc_cmp(crc_result, buf + GXCAS_BLOCK_SIZE - 4);
		backup_sectioin.start_addr = partition_ctrl.backup->start_addr;
		backup_sectioin.total_size = partition_ctrl.backup->total_size;
		backup_sectioin.mtd_id = partition_ctrl.backup->mtd_id;
		backup_sectioin.partition_size = partition_ctrl.backup->partition_size;
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = GxCore_PartitionRead(&backup_sectioin, (void *)buf, GXCAS_BLOCK_SIZE, backup_offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "backup read failed");
			goto err;
		}
		backup_crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);

		if ((buf[GXCAS_BLOCK_SIZE - 4] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 3] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 2] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 1] == 0xff))
			continue;//if backup partition has not been written return success

		if (cmp_value != 0) {
			if (crc_cmp(backup_crc_result, buf + GXCAS_BLOCK_SIZE - 4) != 0) {
				CAS_DBG(FLASH, "Backup Crc Error");
				goto reinit;
			} else {
				ret = GxCore_PartitionWrite(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
				if (ret == 0) {
					CAS_DBG(FLASH, "Recover Original Area Failed");
					goto err;
				}
			}
		}else{
			if((crc_result!=backup_crc_result)&&(crc_cmp(backup_crc_result, buf + GXCAS_BLOCK_SIZE - 4) == 0)){
				ret = GxCore_PartitionWrite(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
				if (ret == 0) {
					CAS_DBG(FLASH, "Recover Original Area Failed");
					goto err;
				}
			}else if(crc_cmp(backup_crc_result, buf + GXCAS_BLOCK_SIZE - 4) != 0){
				memset(buf, 0, GXCAS_BLOCK_SIZE);
				ret = GxCore_PartitionRead(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
				if (ret == 0) {
					CAS_DBG(FLASH, "original Read Failed");
					goto err;
				}
				ret = GxCore_PartitionWrite(&backup_sectioin, (void *)buf, GXCAS_BLOCK_SIZE, backup_offset);
				if (ret == 0) {
					CAS_DBG(FLASH, "Recover Backup Area Failed");
					goto err;
				}
			}
		}
	}
	GxCore_Free(buf);
	CAS_DBG(FLASH,  "Check CRC Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
reinit:
	GxCore_Free(buf);
	CAS_DBG(FLASH,  "CA And Backup Data all Error!!");
	return -2;
}

static int _partition_init(GxCas_FlashConfig flash, GxCas_FlashConfig backup)
{
	int32_t ret = 0;
	struct partition *temp = NULL;
	temp = GxCore_PartitionFlashInit();
	if(NULL == temp) {
		CAS_DBG(FLASH, "GxCore_PartitionFlashInit Failed");
		return -1;
	}

	partition_ctrl.flash = GxCore_PartitionGetByName(temp, flash.name);
	partition_ctrl.size = flash.size;
	partition_ctrl.magic = MAGIC_NUM;

	if (backup.size != 0) {
		partition_ctrl.backup = GxCore_PartitionGetByName(temp, backup.name);
		partition_ctrl.backup_addr = backup.offset;
	}

	if(NULL == partition_ctrl.flash || partition_ctrl.flash->total_size < flash.size ||
			(partition_ctrl.backup != NULL && partition_ctrl.backup->total_size < flash.size)) {
		CAS_DBG(FLASH, "Partition Info Error");
		return -1;
	}

	if (partition_ctrl.backup){
		ret = _partition_check_crc();
		if(ret == -2){
			//TODO: when flash and backup all broken 
			return _partition_reinit_flash_data();
		}
		return ret;
	}else
		return 0;
}

static int32_t _partition_read(uint32_t Offset,
		uint8_t *pbyData,
		size_t dwLen)
{
	int ret = 0;
	struct partition_info user_section;

	memset(&user_section, 0, sizeof(struct partition_info));
	user_section.start_addr = partition_ctrl.flash->start_addr;
	user_section.total_size = partition_ctrl.flash->total_size;
	user_section.partition_size = partition_ctrl.flash->partition_size;
	user_section.mtd_id = partition_ctrl.flash->mtd_id;

	ret = GxCore_PartitionRead(&user_section, (void*)pbyData, dwLen, Offset);
	if(0 == ret)
		return -1;
	else
		return dwLen;
}

static int32_t _partition_update_crc(struct partition_info *partition, uint32_t offset)
{
	struct partition_info section;
	uint32_t ret = 0, crc_result = 0;
	uint8_t *buf = NULL;
	
	buf = GxCore_Mallocz(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	offset = (offset/(GXCAS_BLOCK_SIZE)) * (GXCAS_BLOCK_SIZE);
	section.start_addr = partition->start_addr;
	section.total_size = partition->total_size;
	section.partition_size = partition->partition_size;
	section.mtd_id = partition->mtd_id;
	ret = GxCore_PartitionRead(&section, buf, GXCAS_BLOCK_SIZE, offset);
	if (ret == 0) {
		CAS_DBG(FLASH, "Read Failed");
		goto err;
	}
	crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);
	buf[GXCAS_BLOCK_SIZE - 4] = (crc_result >> 24) & 0xff;
	buf[GXCAS_BLOCK_SIZE - 3] = (crc_result >> 16) & 0xff;
	buf[GXCAS_BLOCK_SIZE - 2] = (crc_result >> 8) & 0xff;
	buf[GXCAS_BLOCK_SIZE - 1] = crc_result & 0xff;
	ret = GxCore_PartitionWrite(&section, buf, GXCAS_BLOCK_SIZE, offset);
	if (ret == 0) {
		CAS_DBG(FLASH, "Write Failed");
		goto err;
	}
	GxCore_Free(buf);
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _partition_write(struct partition_info *partition, uint32_t Offset,
		const uint8_t *pbyData,
		size_t dwLen)
{
	int  ret = 0;
	struct partition_info section;

	section.start_addr = partition->start_addr;
	section.total_size = partition->total_size;
	section.partition_size = partition->partition_size;
	section.mtd_id = partition->mtd_id;
	ret = GxCore_PartitionWrite(&section, (void *)pbyData, dwLen, Offset);
	if(0 == ret) {
		CAS_DBG(FLASH, "Write Block:%d, Failed", Offset/(64 * 1024));
		return -1;
	} else
		return dwLen;
}

static int32_t _partition_check_flash_data(struct partition_info *partition, uint32_t offset)
{
	uint8_t* buf = NULL;
	struct partition_info flash_section;
	uint32_t ret = 0, crc_result = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	offset = (offset/(GXCAS_BLOCK_SIZE)) * (GXCAS_BLOCK_SIZE);
	flash_section.start_addr = partition->start_addr;
	flash_section.total_size = partition->total_size;
	flash_section.partition_size = partition->partition_size;
	flash_section.mtd_id = partition->mtd_id;

	memset(buf, 0, GXCAS_BLOCK_SIZE);
	ret = GxCore_PartitionRead(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
	if (ret == 0) {
		CAS_DBG(FLASH, "Flash Read Failed");
		goto err;
	}

	crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);
	cmp_value = crc_cmp(crc_result, buf + GXCAS_BLOCK_SIZE - 4);
	if(cmp_value!=0){
		CAS_DBG(FLASH, "Crc Error");
		goto err;
	}

	GxCore_Free(buf);
	CAS_DBG(FLASH,  "Write Data Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _partition_copy_bakup_to_ca(void)
{
	uint8_t* buf = NULL;
	struct partition_info flash_section, backup_section;
	uint32_t ret = 0, i = 0, block_count = 0,  crc_result = 0, backup_crc_result = 0, offset = 0, backup_offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	flash_section.start_addr = partition_ctrl.flash->start_addr;
	flash_section.total_size = partition_ctrl.flash->total_size;
	flash_section.partition_size = partition_ctrl.flash->partition_size;
	flash_section.mtd_id = partition_ctrl.flash->mtd_id;
	block_count = partition_ctrl.size /(GXCAS_BLOCK_SIZE);
	for (i = 0; i < block_count; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		backup_offset = offset + partition_ctrl.backup_addr;
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = GxCore_PartitionRead(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "original Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);
		cmp_value = crc_cmp(crc_result, buf + GXCAS_BLOCK_SIZE - 4);
		backup_section.start_addr = partition_ctrl.backup->start_addr;
		backup_section.total_size = partition_ctrl.backup->total_size;
		backup_section.partition_size = partition_ctrl.backup->partition_size;
		backup_section.mtd_id = partition_ctrl.backup->mtd_id;
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = GxCore_PartitionRead(&backup_section, (void *)buf, GXCAS_BLOCK_SIZE, backup_offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "backup read failed");
			goto err;
		}
		backup_crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE - 4);
		if ((buf[GXCAS_BLOCK_SIZE - 4] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 3] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 2] == 0xff)
				&&(buf[GXCAS_BLOCK_SIZE - 1] == 0xff)){
			CAS_DBG(FLASH, "backup partition has not been written");
			continue;//if backup partition has not been written return success
		}

		if (cmp_value != 0) {
			if (crc_cmp(backup_crc_result, buf + GXCAS_BLOCK_SIZE - 4) != 0) {
				CAS_DBG(FLASH, "Backup Crc Error");
				goto err;
			} else {
				ret = GxCore_PartitionWrite(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
				if (ret == 0) {
					CAS_DBG(FLASH, "Recover Original Area Failed");
					goto err;
				}
			}
		}
	}

	GxCore_Free(buf);
	CAS_DBG(FLASH,  "Copy Bakup To Ca Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _partition_reinit_flash_data(void)
{
	uint8_t* buf = NULL;
	struct partition_info flash_section, backup_section;
	uint32_t ret = 0, i = 0, block_count = 0, offset = 0, backup_offset = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	flash_section.start_addr = partition_ctrl.flash->start_addr;
	flash_section.total_size = partition_ctrl.flash->total_size;
	flash_section.partition_size = partition_ctrl.flash->partition_size;
	flash_section.mtd_id = partition_ctrl.flash->mtd_id;
	backup_section.start_addr = partition_ctrl.backup->start_addr;
	backup_section.total_size = partition_ctrl.backup->total_size;
	backup_section.partition_size = partition_ctrl.backup->partition_size;
	backup_section.mtd_id = partition_ctrl.backup->mtd_id;
	block_count = partition_ctrl.size /(GXCAS_BLOCK_SIZE);
	for (i = 0; i < block_count; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		backup_offset = offset + partition_ctrl.backup_addr;
		memset(buf, 0xFF, GXCAS_BLOCK_SIZE);
		ret = GxCore_PartitionWrite(&flash_section, (void *)buf, GXCAS_BLOCK_SIZE, offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "flash write failed");
			goto err;
		}
		ret = GxCore_PartitionWrite(&backup_section, (void *)buf, GXCAS_BLOCK_SIZE, backup_offset);
		if (ret == 0) {
			CAS_DBG(FLASH, "backup write failed");
			goto err;
		}
	}

	GxCore_Free(buf);
	CAS_DBG(FLASH,  "Reinit Flash Data Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _mix_nvram_open(const char* name)
{
	uint8_t *value = NULL;
	handle_t handle = -1;
	uint32_t ret = 0;

	if (GxCore_FileExists(name) == GXCORE_MOD_EXIST) {
		CAS_DBG(FLASH,"File is exist!");
		return 0;
	}

	CAS_DBG(FLASH,"File is not exist!Creat & clear it now");
	value = GxCore_Malloc(mixflash_ctrl.size);
	if(value == NULL) {
		CAS_ERR(FLASH,"GxCore_Malloc memory failure!");
		goto err;
	}
	memset(value, 0xFF, mixflash_ctrl.size);

	handle = GxCore_Open(name, "c");
	GxCore_Seek(handle, 0, GX_SEEK_SET);
	ret = GxCore_Write(handle, value,1, mixflash_ctrl.size);
	if(ret != mixflash_ctrl.size) {
		CAS_ERR(FLASH,"Write file err! ret size = %d, write size = %d", ret, mixflash_ctrl.size);
		goto err;
	}
	GxCore_Close(handle);
	GxCore_Free(value);

	//return _nvram_update_crc(name);
	return 0;
err:
	if (value)
		GxCore_Free(value);
	if (handle != -1)
		GxCore_Close(handle);
	return -1;
}

static int32_t _mix_nvram_update_crc(const char *name, uint32_t offset, size_t len)
{
	handle_t handle = -1;
	char *value = NULL;
	uint8_t crc_buf[4] = {0,};
	uint32_t ret = 0, crc = 0, block_count = 0;
	uint32_t num = 0, i = 0;
	
	GxCore_MutexLock(mixflash_ctrl.lock);
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	if(offset%(GXCAS_BLOCK_SIZE) == 0){
		num = len/GXCAS_BLOCK_SIZE;
		if((len%GXCAS_BLOCK_SIZE) > 0)
			num++;
	}else{
		if(len <= (GXCAS_BLOCK_SIZE - offset%GXCAS_BLOCK_SIZE))
			num = 1;
		else{
			num = (len - (GXCAS_BLOCK_SIZE - offset%GXCAS_BLOCK_SIZE))/GXCAS_BLOCK_SIZE;
			if(((len - offset%(GXCAS_BLOCK_SIZE))%GXCAS_BLOCK_SIZE) > 0)
				num++;
			num++;
		}
	}
	offset = offset/(GXCAS_BLOCK_SIZE);
	if(num + offset >= block_count){
		CAS_ERR(FLASH, "num:%d + offset:%d >= block_count:%d", num, offset, block_count);
		goto err;
	}
	handle = GxCore_Open(name, "r+");
	value = GxCore_Mallocz(GXCAS_BLOCK_SIZE);
	if (value == NULL)
		goto err;
	for(i = offset; i < num + offset; i++){
		GxCore_Seek(handle, (long)(i * GXCAS_BLOCK_SIZE), GX_SEEK_SET);
		ret = GxCore_Read(handle, value, 1, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "Read len != GXCAS_BLOCK_SIZE");
			goto err;
		}

		crc = GxCas_Crc32((uint8_t *)value, GXCAS_BLOCK_SIZE);
		crc_buf[0] = (crc >> 24) & 0xff;
		crc_buf[1] = (crc >> 16) & 0xff;
		crc_buf[2] = (crc >> 8) & 0xff;
		crc_buf[3] = (crc) & 0xff;
		GxCore_Seek(handle, (long)(GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE), GX_SEEK_SET);
		ret = GxCore_Write(handle, crc_buf, 1, GXCAS_CRC_SIZE);
		if (ret != GXCAS_CRC_SIZE) {
			CAS_DBG(FLASH, "Write len != GXCAS_CRC_SIZE");
			goto err;
		}
	}
	GxCore_Close(handle);
	GxCore_Free(value);
	GxCore_MutexUnlock(mixflash_ctrl.lock);
	return 0;
err:
	if (value)
		GxCore_Free(value);
	if (handle != -1)
		GxCore_Close(handle);
	GxCore_MutexUnlock(mixflash_ctrl.lock);
	return -1;
}

static size_t _mix_nvram_read(uint32_t offset, uint8_t *buf, size_t len)
{
	handle_t handle;
	size_t   size = 0;
	CAS_DBG(FLASH,"Call.%s",__FUNCTION__);

	if((buf == NULL) || (offset + len > mixflash_ctrl.size)){
		CAS_ERR(FLASH,"buff is NULL");
		return -1;
	}

	GxCore_MutexLock(mixflash_ctrl.lock);
	handle = GxCore_Open((char*)mixflash_ctrl.name_backup, "r");
	GxCore_Seek(handle, (long)offset, GX_SEEK_SET);
	size = GxCore_Read(handle, buf, 1,len);
	if(size != len) {
		CAS_ERR(FLASH,"Read file err! ret size = %d, read size = %d",size,len);
		size = -1;
	}
	GxCore_Close(handle);
	GxCore_MutexUnlock(mixflash_ctrl.lock);

	return size;
}

static size_t _mix_nvram_write(const char* name, uint32_t offset, const uint8_t *buf, size_t len)
{
	handle_t handle;
	size_t   size = 0;

	if((buf == NULL) || (offset + len > mixflash_ctrl.size) || strlen(name) == 0) {
		CAS_ERR(FLASH,"buff is NULL");
		return -1;
	}

	GxCore_MutexLock(mixflash_ctrl.lock);
	handle = GxCore_Open((char*)name, "r+");
	GxCore_Seek(handle, (long)offset, GX_SEEK_SET);
	size = GxCore_Write(handle, (uint8_t*)buf, 1,len);
	if(size != len) {
		CAS_ERR(FLASH,"Write file err! ret size = %d, write size = %d",size,len);
		size = -1;
	}
	GxCore_Close(handle);
	GxCore_MutexUnlock(mixflash_ctrl.lock);
	return size;
}

static int32_t _mix_partition_update_crc(struct partition_info *partition, uint32_t offset, size_t len)
{
	struct partition_info section;
	uint32_t ret = 0, crc_result = 0, block_count = 0;
	uint8_t *buf = NULL;
	uint8_t crc_buf[4] = {0,};
	uint32_t num = 0, i = 0;
	
	buf = GxCore_Mallocz(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	if(offset%(GXCAS_BLOCK_SIZE) == 0){
		num = len/GXCAS_BLOCK_SIZE;
		if((len%GXCAS_BLOCK_SIZE) > 0)
			num++;
	}else{
		if(len <= (GXCAS_BLOCK_SIZE - offset%GXCAS_BLOCK_SIZE))
			num = 1;
		else{
			num = (len - (GXCAS_BLOCK_SIZE - offset%GXCAS_BLOCK_SIZE))/GXCAS_BLOCK_SIZE;
			if(((len - offset%(GXCAS_BLOCK_SIZE))%GXCAS_BLOCK_SIZE) > 0)
				num++;
			num++;
		} 
	}
	offset = offset/(GXCAS_BLOCK_SIZE);
	if(num + offset >= block_count){
		CAS_ERR(FLASH, "num:%d + offset:%d >= block_count:%d", num, offset, block_count);
		goto err;
	}
	section.start_addr = partition->start_addr;
	section.total_size = partition->total_size;
	section.partition_size = partition->partition_size;
	section.mtd_id = partition->mtd_id;
	for(i = offset; i < num + offset; i++){
		ret = GxCore_PartitionRead(&section, buf, GXCAS_BLOCK_SIZE, i * GXCAS_BLOCK_SIZE);
		if (ret == 0) {
			CAS_DBG(FLASH, "Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		crc_buf[0] = (crc_result >> 24) & 0xff;
		crc_buf[1] = (crc_result >> 16) & 0xff;
		crc_buf[2] = (crc_result >> 8) & 0xff;
		crc_buf[3] = crc_result & 0xff;
		ret = GxCore_PartitionWrite(&section, crc_buf, GXCAS_CRC_SIZE, GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE);
		if (ret == 0) {
			CAS_DBG(FLASH, "Write Failed");
			goto err;
		}
	}

	GxCore_Free(buf);
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _mix_partition_read(uint32_t Offset,
		uint8_t *pbyData,
		size_t dwLen)
{
	int ret = 0;
	struct partition_info user_section;

	memset(&user_section, 0, sizeof(struct partition_info));
	user_section.start_addr = mixflash_ctrl.flash->start_addr;
	user_section.total_size = mixflash_ctrl.flash->total_size;
	user_section.partition_size = mixflash_ctrl.flash->partition_size;
	user_section.mtd_id = mixflash_ctrl.flash->mtd_id;

	ret = GxCore_PartitionRead(&user_section, (void*)pbyData, dwLen, Offset);
	if(0 == ret)
		return -1;
	else
		return dwLen;
}

static int32_t _mix_partition_write(struct partition_info *partition, uint32_t Offset,
		const uint8_t *pbyData,
		size_t dwLen)
{
	int  ret = 0;
	struct partition_info section;

	section.start_addr = partition->start_addr;
	section.total_size = partition->total_size;
	section.partition_size = partition->partition_size;
	section.mtd_id = partition->mtd_id;
	section.partition_size = partition->partition_size;
	ret = GxCore_PartitionWrite(&section, (void *)pbyData, dwLen, Offset);
	if(0 == ret) {
		CAS_DBG(FLASH, "Write Block:%d, Failed", Offset/(64 * 1024));
		return -1;
	} else
		return dwLen;
}

static int32_t _mix_recover_original(uint32_t offset, uint32_t crc_offset, uint32_t crc_sn, uint8_t* buf, uint8_t* crc_buf)
{
	uint32_t ret = 0;
	ret = _mix_partition_write(mixflash_ctrl.flash, offset, buf, GXCAS_BLOCK_SIZE);
	if (ret != GXCAS_BLOCK_SIZE) {
		CAS_DBG(FLASH, "Recover Original Area Failed");
		return -1;
	}
	ret = _mix_partition_write(mixflash_ctrl.flash, crc_offset, crc_buf + crc_sn * GXCAS_CRC_SIZE, GXCAS_CRC_SIZE);
	if (ret != GXCAS_CRC_SIZE) {
		CAS_DBG(FLASH, "Recover Original Crc Area Failed");
		return -1;
	}
	return 0;
}

static int32_t _mix_recover_backup(uint32_t offset, uint32_t crc_offset, uint32_t crc_sn, uint8_t* buf, uint8_t* crc_buf)
{
	uint32_t ret = 0;
	ret = _mix_nvram_write(mixflash_ctrl.name_backup, offset, buf, GXCAS_BLOCK_SIZE);
	if (ret != GXCAS_BLOCK_SIZE) {
		CAS_DBG(FLASH, "Recover Backup Area Failed");
		return -1;
	}
	ret = _mix_nvram_write(mixflash_ctrl.name_backup, crc_offset, crc_buf + crc_sn * GXCAS_CRC_SIZE, GXCAS_CRC_SIZE);
	if (ret != GXCAS_CRC_SIZE) {
		CAS_DBG(FLASH, "Recover Backup Crc Area Failed");
		return -1;
	}
	return 0;
}

static int32_t _mix_copy_bakup_to_ca(void)
{
	uint8_t* buf = NULL, *crc_buf = NULL, *crc_back_buf = NULL;
	uint32_t ret = 0, i = 0, block_count = 0,  crc_result = 0, backup_crc_result = 0, offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	// get original crc 
	crc_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_buf == NULL)
		goto err;
	memset(crc_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_partition_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "original Read crc data Failed");
		goto err;
	}
	// get backup crc 
	crc_back_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_back_buf == NULL)
		goto err;
	memset(crc_back_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_nvram_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_back_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "backup crc read size != crc block_count");
		goto err;
	}
	
	for (i = 0; i < block_count - 1; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_partition_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "original Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		cmp_value = crc_cmp(crc_result, crc_buf + i * GXCAS_CRC_SIZE);
		
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_nvram_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "backup read size != GXCAS_BLOCK_SIZE");
			goto err;
		}
		backup_crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		if ((crc_back_buf[i * GXCAS_CRC_SIZE] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 1] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 2] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 3] == 0xff)){
			CAS_DBG(FLASH, "backup partition has not been written");
			continue;//if backup and original area not be written return Success
		}

		if (cmp_value != 0) {
			if (crc_cmp(backup_crc_result, crc_back_buf + i * GXCAS_CRC_SIZE) != 0) {
				CAS_DBG(FLASH, "Backup Crc Error");
				goto err;
			} else {
				ret = _mix_recover_original(offset, GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE, i, buf, crc_back_buf); 
				if (ret != 0) {
					goto err;
				}
			}
		}
	}
	GxCore_Free(buf);
	GxCore_Free(crc_buf);
	GxCore_Free(crc_back_buf);
	CAS_DBG(FLASH,  "Copy Bakup To Ca Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (crc_buf)
		GxCore_Free(crc_buf);
	if (crc_back_buf)
		GxCore_Free(crc_back_buf);
	return -1;
}

static int32_t _mix_reinit_flash_data(void)
{
	uint8_t* buf = NULL;
	uint32_t ret = 0, i = 0, block_count = 0, offset = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);

	for (i = 0; i < block_count - 1; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		memset(buf, 0xFF, GXCAS_BLOCK_SIZE);
		ret = _mix_partition_write(mixflash_ctrl.flash, offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "Original Write Failed");
			goto err;
		}
	}
	if (strlen(mixflash_ctrl.name_backup) != 0) {
		GxCore_FileDelete(mixflash_ctrl.name_backup);
		if (_mix_nvram_open(mixflash_ctrl.name_backup) < 0)
			goto err;
	}
	GxCore_Free(buf);
	CAS_DBG(FLASH,  "Reinit Flash Data Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	return -1;
}

static int32_t _mix_nvram_check_flash_data(void)
{
	uint8_t* buf = NULL, *crc_back_buf = NULL;
	uint32_t ret = 0, i = 0, block_count = 0, backup_crc_result = 0, offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	// get backup crc 
	crc_back_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_back_buf == NULL)
		goto err;
	memset(crc_back_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_nvram_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_back_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "backup crc read size != crc block_count");
		goto err;
	}
	
	for (i = 0; i < block_count - 1; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_nvram_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "backup read size != GXCAS_BLOCK_SIZE");
			goto err;
		}
		backup_crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		if((crc_back_buf[i * GXCAS_CRC_SIZE] == 0xff)
			&&(crc_back_buf[i * GXCAS_CRC_SIZE + 1] == 0xff)
			&&(crc_back_buf[i * GXCAS_CRC_SIZE + 2] == 0xff)
			&&(crc_back_buf[i * GXCAS_CRC_SIZE + 3] == 0xff))
			continue;
		cmp_value = crc_cmp(backup_crc_result, crc_back_buf + i * GXCAS_CRC_SIZE);

		if (cmp_value != 0) {
			CAS_DBG(FLASH, "Backup Crc Error");
			goto err;
		}
	}
	GxCore_Free(buf);
	GxCore_Free(crc_back_buf);
	CAS_DBG(FLASH,  "Check Backup Data Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (crc_back_buf)
		GxCore_Free(crc_back_buf);
	return -1;
}

static int32_t _mix_partition_check_flash_data(void)
{
	uint8_t* buf = NULL, *crc_buf = NULL;
	uint32_t ret = 0, i = 0, block_count = 0, crc_result = 0, offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	// get original crc 
	crc_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_buf == NULL)
		goto err;
	memset(crc_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_partition_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "original Read crc data Failed");
		goto err;
	}

	for (i = 0; i < block_count - 1; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_partition_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "original Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		if((crc_buf[i * GXCAS_CRC_SIZE] == 0xff)
			&&(crc_buf[i * GXCAS_CRC_SIZE + 1] == 0xff)
			&&(crc_buf[i * GXCAS_CRC_SIZE + 2] == 0xff)
			&&(crc_buf[i * GXCAS_CRC_SIZE + 3] == 0xff))
			continue;
		cmp_value = crc_cmp(crc_result, crc_buf + i * GXCAS_CRC_SIZE);

		if (cmp_value != 0) {
			CAS_DBG(FLASH, "Ca Crc Error");
			goto err;
		}
	}
	GxCore_Free(buf);
	GxCore_Free(crc_buf);
	CAS_DBG(FLASH,  "Check Ca Data Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (crc_buf)
		GxCore_Free(crc_buf);
	return -1;
}

static int32_t _mixflash_check_crc()
{
	uint8_t* buf = NULL, *crc_buf = NULL, *crc_back_buf = NULL;
	uint32_t ret = 0, i = 0, block_count = 0,  crc_result = 0, backup_crc_result = 0, offset = 0;
	int  cmp_value = 0;

	buf = GxCore_Malloc(GXCAS_BLOCK_SIZE);
	if (buf == NULL)
		goto err;
	block_count = mixflash_ctrl.size /(GXCAS_BLOCK_SIZE);
	// get original crc 
	crc_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_buf == NULL)
		goto err;
	memset(crc_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_partition_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "original Read crc data Failed");
		goto err;
	}
	// get backup crc 
	crc_back_buf = GxCore_Malloc(GXCAS_CRC_SIZE * (block_count - 1));
	if (crc_back_buf == NULL)
		goto err;
	memset(crc_back_buf, 0, GXCAS_CRC_SIZE * (block_count - 1));
	ret = _mix_nvram_read(GXCAS_BLOCK_SIZE * (block_count - 1), crc_back_buf, GXCAS_CRC_SIZE * (block_count - 1));
	if (ret != GXCAS_CRC_SIZE * (block_count - 1)) {
		CAS_DBG(FLASH, "backup crc read size != crc block_count");
		goto err;
	}
	
	for (i = 0; i < block_count - 1; i++) {
		offset = GXCAS_BLOCK_SIZE * i; //Block A B C
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_partition_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "original Read Failed");
			goto err;
		}
		crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);
		cmp_value = crc_cmp(crc_result, crc_buf + i * GXCAS_CRC_SIZE);
		
		memset(buf, 0, GXCAS_BLOCK_SIZE);
		ret = _mix_nvram_read(offset, buf, GXCAS_BLOCK_SIZE);
		if (ret != GXCAS_BLOCK_SIZE) {
			CAS_DBG(FLASH, "backup read size != GXCAS_BLOCK_SIZE");
			goto err;
		}
		backup_crc_result = GxCas_Crc32(buf, GXCAS_BLOCK_SIZE);

		if ((crc_back_buf[i * GXCAS_CRC_SIZE] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 1] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 2] == 0xff)
				&&(crc_back_buf[i * GXCAS_CRC_SIZE + 3] == 0xff)
				&&(crc_buf[i * GXCAS_CRC_SIZE] == 0xff)
				&&(crc_buf[i * GXCAS_CRC_SIZE + 1] == 0xff)
				&&(crc_buf[i * GXCAS_CRC_SIZE + 2] == 0xff)
				&&(crc_buf[i * GXCAS_CRC_SIZE + 3] == 0xff))
			continue;//if backup and original area not be written return success

		if (cmp_value != 0) {
			if (crc_cmp(backup_crc_result, crc_back_buf + i * GXCAS_CRC_SIZE) != 0) {
				CAS_DBG(FLASH, "Backup Crc Error");
				goto reinit;
			} else {
				ret = _mix_recover_original(offset, GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE, i, buf, crc_back_buf); 
				if (ret != 0) {
					goto err;
				}
			}
		}else{
			if((crc_result!=backup_crc_result)&&(crc_cmp(backup_crc_result, crc_back_buf + i * GXCAS_CRC_SIZE) == 0)){
				ret = _mix_recover_original(offset, GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE, i, buf, crc_back_buf); 
				if (ret != 0) {
					goto err;
				}
			} else if(crc_cmp(backup_crc_result, crc_back_buf + i * GXCAS_CRC_SIZE) != 0){
				memset(buf, 0, GXCAS_BLOCK_SIZE);
				ret = _mix_partition_read(offset, buf, GXCAS_BLOCK_SIZE);
				if (ret != GXCAS_BLOCK_SIZE) {
					CAS_DBG(FLASH, "original Read Failed");
					goto err;
				}
				ret = _mix_recover_backup(offset, GXCAS_BLOCK_SIZE * (block_count - 1) + i * GXCAS_CRC_SIZE, i, buf, crc_buf); 
				if (ret != 0) {
					goto err;
				}
			}
		}
	}
	GxCore_Free(buf);
	GxCore_Free(crc_buf);
	GxCore_Free(crc_back_buf);
	CAS_DBG(FLASH,  "Check CRC Success");
	return 0;
err:
	if (buf)
		GxCore_Free(buf);
	if (crc_buf)
		GxCore_Free(crc_buf);
	if (crc_back_buf)
		GxCore_Free(crc_back_buf);
	return -1;
reinit:
	if (buf)
		GxCore_Free(buf);
	if (crc_buf)
		GxCore_Free(crc_buf);
	if (crc_back_buf)
		GxCore_Free(crc_back_buf);
	CAS_DBG(FLASH,  "CA And Backup Data all Error!!");
	return -2;
}

static int _mixflash_init(GxCas_FlashConfig flash, GxCas_FlashConfig backup)
{
	int32_t ret = 0, ret1 = 0;
	uint8_t count = 0;
	struct partition *temp = NULL;
	temp = GxCore_PartitionFlashInit();
	if(NULL == temp) {
		CAS_DBG(FLASH, "GxCore_PartitionFlashInit Failed");
		return -1;
	}

	mixflash_ctrl.magic = MAGIC_NUM;
	mixflash_ctrl.flash = GxCore_PartitionGetByName(temp, flash.name);
	mixflash_ctrl.size = flash.size;
	sprintf(mixflash_ctrl.name_backup, "%s", backup.name);
	mixflash_ctrl.start_addr = 0x10000;// for dvtca,if start_addr is 0, dvtca has error 

	if(NULL == mixflash_ctrl.flash || mixflash_ctrl.flash->total_size < flash.size) {
		CAS_DBG(FLASH, "Partition Info Error");
		goto err;
	}

init:
	count++;
	if(backup.size){
		if (_mix_nvram_open(mixflash_ctrl.name_backup) < 0)
			goto rebuild;

		ret = _mixflash_check_crc();
		if(ret == -2){
			//TODO: when flash and backup all broken reinit
			if(_mix_reinit_flash_data()<0)
				goto rebuild;
		}else if(ret < 0)
			goto rebuild;
	}
	if(0 != GxCore_MutexCreate(&mixflash_ctrl.lock)) {
		CAS_ERR(FLASH,"Creat mutex failure!");
		goto err;
	}

	return 0;
err:
	return -1;
rebuild:
	if(backup.func == NULL){
		CAS_ERR(FLASH,"Use default Rebuild minifs cas backup func!");
		backup.func = _rebuild_minifs_func;
	}
	if(count <= 3){
		CAS_ERR(FLASH,"Rebuild minifs cas backup partition!!! count = %d", count);
		ret1 = backup.func();
		if(ret1 < 0){
			CAS_ERR(FLASH,"rebuild_minifs_func failed!!! ret = %d", ret1);
			if(ret1 == -1){
				count++;
				goto rebuild;
			}else// when ret1 = -2, umount or mount failed, hot reset can not rebuild minif, so need to while(1), to tell customer reboot.
				return -1;
		}else
			goto init;
	}else{
		CAS_ERR(FLASH,"Rebuild minifs cas backup partition failure!");
		return -1;
	}
	return 0;
}

#ifdef CONFIG_ASYNC_FLASH_WRITE
static int32_t _partition_async_write(struct partition_info *partition, uint32_t Offset,
		const uint8_t *data,
		size_t len)
{
	int  ret = 0;
	flash_async_node node;
	memset(&node, 0, sizeof(flash_async_node));
	node.flash = partition;
	node.offset = Offset;
	node.data = (uint8_t*)data;
	node.len = len;
	node.crc_flag = 0;
	ret = GxFlashAsync_Add(&node);
	if(ret != 0)
		CAS_DBG(FLASH, "GxFlashAsync_Add Failed!!!");
	return ret;
}
static int32_t _partition_async_update_crc(struct partition_info *partition, uint32_t offset)
{
	int  ret = 0;
	flash_async_node node;
	memset(&node, 0, sizeof(flash_async_node));
	node.flash = partition;
	node.offset = offset;
	node.crc_flag = 1;
	ret = GxFlashAsync_Add(&node);
	if(ret != 0)
		CAS_DBG(FLASH, "GxFlashAsync_Add Failed!!!");
	return ret;
}
#endif
int32_t GxCas_Flash_Init(GxCas_FlashConfig flash, GxCas_FlashConfig backup)
{
	int32_t ret = -1;
	if ((strlen(flash.name) == 0) || (backup.size != 0 && flash.size != backup.size)) {
		CAS_DBG(FLASH, "Flash Name Error");
		goto err;
	}

	if (strstr(flash.name, ".dat")) {
		//not support backup in the same file
		if (strcmp(flash.name, backup.name) == 0) {
			CAS_DBG(FLASH, "Flash Name == Backup Name:%s", flash.name);
			goto err;
		}
		ret = _nvram_init(flash, backup);
	} else {
		if ((backup.size != 0) && (strstr(backup.name, ".dat"))) {
			ret = _mixflash_init(flash, backup);
		} else{
			//if partition ,backup area can not cover flash area
			if ((strcmp(flash.name, backup.name) == 0) && (abs(flash.offset - backup.offset) < flash.size)) {
				CAS_DBG(FLASH, "Flash Offset:%d, Backup Offset:%d",flash.offset, backup.offset);
				goto err;
			}
			ret = _partition_init(flash, backup);
#ifdef CONFIG_ASYNC_FLASH_WRITE
			GxFlashAsync_Init();
#endif
		}
	}

	return ret;
err:
	return -1;
}

int32_t GxCas_Flash_Read(uint32_t offset, uint8_t *buf, size_t len)
{
	int32_t ret = -1;
	if (partition_ctrl.magic == MAGIC_NUM)
		ret = _partition_read(offset, buf, len);
	else if (nvram_ctrl.magic == MAGIC_NUM)
		ret =  _nvram_read(offset, buf, len);
	else if (mixflash_ctrl.magic == MAGIC_NUM)
		ret =  _mix_partition_read(offset, buf, len);
	return ret;
}

int32_t GxCas_Flash_Write(uint32_t offset, const uint8_t *buf, size_t len)
{
	int32_t ret = -1;
	if (partition_ctrl.magic == MAGIC_NUM) {
		if (partition_ctrl.backup) {
			if(_partition_check_flash_data(partition_ctrl.flash, offset) == -1){
				if(_partition_copy_bakup_to_ca() == -1){
					goto err;
				}
			}
			if ((ret = _partition_write(partition_ctrl.backup, offset + partition_ctrl.backup_addr, buf, len) != len))
				goto err;
			if (_partition_update_crc(partition_ctrl.backup, offset + partition_ctrl.backup_addr) == -1)
				goto err;
			//TODO: check flash content by crc
			if(_partition_check_flash_data(partition_ctrl.backup, offset + partition_ctrl.backup_addr) == -1)
				goto err;
		}
		if ((ret = _partition_write(partition_ctrl.flash, offset, buf, len)) != len)
			goto err;
		if (partition_ctrl.backup && (_partition_update_crc(partition_ctrl.flash, offset) == -1))
			goto err;
		if((partition_ctrl.backup) && (_partition_check_flash_data(partition_ctrl.flash, offset) == -1))
			goto err;
	} else if (nvram_ctrl.magic == MAGIC_NUM) {
		if (strlen(nvram_ctrl.name_backup) != 0) {
			if(_nvram_check_flash_data(nvram_ctrl.name) == -1){
				if(_nvram_copy_bakup_to_ca() == -1){
					goto err;
				}
			}
			if ((ret = _nvram_write(nvram_ctrl.name_backup, offset, buf, len)) != len)
				goto err;
			if (_nvram_update_crc(nvram_ctrl.name_backup) == -1)
				goto err;
			//TODO: check flash content by crc
			if(_nvram_check_flash_data(nvram_ctrl.name_backup) == -1)
				goto err;
		}
		if ((ret = _nvram_write(nvram_ctrl.name, offset, buf, len)) != len)
			goto err;
		if (strlen(nvram_ctrl.name_backup) != 0 && _nvram_update_crc(nvram_ctrl.name) == -1)
			goto err;
		if((strlen(nvram_ctrl.name_backup) != 0) && (_nvram_check_flash_data(nvram_ctrl.name) == -1))
			goto err;
	} else if (mixflash_ctrl.magic == MAGIC_NUM) {
		if (strlen(mixflash_ctrl.name_backup) != 0) {
			if(_mix_partition_check_flash_data() == -1){
				if(_mix_copy_bakup_to_ca() == -1){
					goto err;
				}
			}
			if ((ret = _mix_nvram_write(mixflash_ctrl.name_backup, offset, buf, len)) != len)
				goto err;
			if (_mix_nvram_update_crc(mixflash_ctrl.name_backup, offset, len) == -1)
				goto err;
			//TODO: check flash content by crc
			if(_mix_nvram_check_flash_data() == -1)
				goto err;
		}
		if ((ret = _mix_partition_write(mixflash_ctrl.flash, offset, buf, len)) != len)
			goto err;
		if (strlen(mixflash_ctrl.name_backup) != 0 && _mix_partition_update_crc(mixflash_ctrl.flash, offset, len) == -1)
			goto err;
		if((strlen(mixflash_ctrl.name_backup) != 0) && (_mix_partition_check_flash_data() == -1))
			goto err;
	}
	return ret;
err:
	return -1;
}
#ifdef CONFIG_ASYNC_FLASH_WRITE
int32_t GxCas_Flash_Async_Write(uint32_t offset, const uint8_t *buf, size_t len)
{
	int32_t ret = -1;
	if (partition_ctrl.magic == MAGIC_NUM) {
		if (partition_ctrl.backup) {
			if ((ret = _partition_async_write(partition_ctrl.backup, offset + partition_ctrl.backup_addr, buf, len) != 0))
				goto err;
			if (_partition_async_update_crc(partition_ctrl.backup, offset + partition_ctrl.backup_addr) == -1)
				goto err;
		}
		if ((ret = _partition_async_write(partition_ctrl.flash, offset, buf, len)) != 0)
			goto err;
		if (partition_ctrl.backup && (_partition_async_update_crc(partition_ctrl.flash, offset) == -1))
			goto err;
	} else if (nvram_ctrl.magic == MAGIC_NUM) {
		if (strlen(nvram_ctrl.name_backup) != 0) {
			if(_nvram_check_flash_data(nvram_ctrl.name) == -1){
				if(_nvram_copy_bakup_to_ca() == -1){
					goto err;
				}
			}
			if ((ret = _nvram_write(nvram_ctrl.name_backup, offset, buf, len)) != len)
				goto err;
			if (_nvram_update_crc(nvram_ctrl.name_backup) == -1)
				goto err;
			if(_nvram_check_flash_data(nvram_ctrl.name_backup) == -1)
				goto err;
		}
		if ((ret = _nvram_write(nvram_ctrl.name, offset, buf, len)) != len)
			goto err;
		if (strlen(nvram_ctrl.name_backup) != 0 && _nvram_update_crc(nvram_ctrl.name) == -1)
			goto err;
		if((strlen(nvram_ctrl.name_backup) != 0) && (_nvram_check_flash_data(nvram_ctrl.name) == -1))
			goto err;
	} else if (mixflash_ctrl.magic == MAGIC_NUM) {
		if (strlen(mixflash_ctrl.name_backup) != 0) {
			if(_mix_partition_check_flash_data() == -1){
				if(_mix_copy_bakup_to_ca() == -1){
					goto err;
				}
			}
			if ((ret = _mix_nvram_write(mixflash_ctrl.name_backup, offset, buf, len)) != len)
				goto err;
			if (_mix_nvram_update_crc(mixflash_ctrl.name_backup, offset, len) == -1)
				goto err;
			if(_mix_nvram_check_flash_data() == -1)
				goto err;
		}
		if ((ret = _mix_partition_write(mixflash_ctrl.flash, offset, buf, len)) != len)
			goto err;
		if (strlen(mixflash_ctrl.name_backup) != 0 && _mix_partition_update_crc(mixflash_ctrl.flash, offset, len) == -1)
			goto err;
		if((strlen(mixflash_ctrl.name_backup) != 0) && (_mix_partition_check_flash_data() == -1))
			goto err;
	}
	return ret;
err:
	return -1;
}
#endif

int32_t GxCas_Flash_GetInfo(uint32_t* start_addr, size_t *size)
{
	int32_t ret = -1;
	if (partition_ctrl.magic == MAGIC_NUM){
		*start_addr = partition_ctrl.flash->start_addr;
		*size = partition_ctrl.flash->total_size;
		ret = 0;
	} else if (nvram_ctrl.magic == MAGIC_NUM){
		*start_addr = nvram_ctrl.start_addr;
		*size = nvram_ctrl.size;
		ret = 0;
	} else {
		*start_addr = 0;
		*size = 0;
	}
	return ret;
}
