#include <math.h>
#include "common/libini.h"
#include "common/gxmalloc.h"
#include "common/gx_crc32.h"
#include "common/gxoem.h"
#include "gxos/gxcore_os_core.h"
#include "ini.h"
#include "gxos/gxcore_partition.h"
#include "config.h"
#include "devapi/gxflash_api.h"

#define OEM_HEARDER "[variable_oem]"

#ifdef CONFIG_ENABLE_MINIFS
#define MINIFS_CRC_FILE_INITIAL_VALUE "[OEM]\nCRC32=FFFFFFFF\n[BACK_OEM]\nCRC32=FFFFFFFF\n"
#ifdef NO_OS
#include "minifs/api_minifs.h"

static minifs_handle_dev  oem_device = NULL;
static minifs_handle_file oem_file = NULL;
static minifs_handle_file backoem_file = NULL;
static minifs_handle_file crc_file = NULL;
#else
#include "gxcore_os_filesys.h"
static handle_t oem_file;
static handle_t backoem_file;
static handle_t crc_file;
#endif
static handle_t c_ini = 0;
static bool g_oem_use_minifs = false;
static bool g_backoem_use_minifs = false;
#endif /*CONFIG_ENABLE_MINIFS*/
GxPartitionTable *flash_partition_table = NULL;
GxPartition *oem_partition = NULL;
GxPartition *backoem_partition = NULL;

static int g_oem_need_backup = 1;

#define INI_DEFAULT_BLOCK_SIZE 4096
static unsigned int ini_block_size = INI_DEFAULT_BLOCK_SIZE;
static handle_t v_ini = 0;
static handle_t ini_mutex = 0;

#define MAX_OEM_FILE_NAME_LEN 100

static char oem_file_name[MAX_OEM_FILE_NAME_LEN+1];
static char backoem_file_name[MAX_OEM_FILE_NAME_LEN+1];

#if 0 /* 先注释掉,后面如果需要备份分区表并且更新分区信息再打开 */
static int ini_flash_partition_table_update(GxPartition* partition)
{
	int32_t i = 0;

	for (i = 0; i < flash_partition_table->count; i++) {
		if (strncasecmp(partition->name, flash_partition_table->tables[i].name, FLASH_PARTITION_NAME_SIZE) == 0) {
			memcpy(&flash_partition_table->tables[i], partition, sizeof(GxPartition));
			GxCore_PartitionTableSave(flash_partition_table);

			return 0;
		}
	}

	return -1;
}
#endif

/* 此函数只能用于存放ascii字符的分区 */
static int32_t ini_calc_oem_patition_used_size(char* partition_buf, int32_t total_size)
{
	int32_t i = 0;
	int32_t used_size = 0;

	for (i = 0; i < total_size; i++) {
		if (partition_buf[i] == 0xFF)
			break;

		used_size++;
	}

	return used_size;
}

/* 此函数只能用于存放ascii字符的分区 */
static int32_t ini_write_oem_partition(GxPartition* partition, char* buf, int32_t size)
{
	int ret = 0;
	char *oem_buf = NULL;

	if (size > (ini_block_size - 5)) {
		gxloge("oem size is larger than %d - 5\n", ini_block_size);
		ret = -1;
		return ret;
	}

	oem_buf = GxCore_Malloc(ini_block_size);

	if (oem_buf == NULL) {
		ret = -1;
		return ret;
	}

	GxCore_PartitionRead(partition, oem_buf, ini_block_size, 0);
	memcpy(oem_buf, buf, size);
	oem_buf[size] = 0xFF; //Use 0xFF as the terminator
	GxCore_PartitionErase(partition);
	if (GxCore_PartitionWriteRaw(partition, oem_buf, ini_block_size, 0) != 1) {
		ret = -1;
		gxloge("write oem error\n");
	}

	GxCore_Free(oem_buf);

	return ret;
}

#ifdef CONFIG_ENABLE_MINIFS
#ifdef NO_OS
minifs_handle_dev gxopen(char *partition_name,char *file_name)
{
	static GxPartitionTable *flash_table = NULL;
	static GxPartition *partition_data = NULL;
	minifs_handle_dev fd;
	if (flash_table == NULL)
		flash_table = GxCore_PartitionFlashInit();

	if(partition_data == NULL) {
		partition_data = GxCore_PartitionGetByName((GxPartitionTable *)flash_table, partition_name);
		if(partition_data == NULL) {
			gxloge(" partition data byname err\n");
			goto err;
		}
	}

	if(partition_data->file_system_type != MINIFS){
		gxloge("this partition fs is not minifs\n");
		goto err;
	}

	if(oem_device == NULL)
		oem_device = minifs_mount(partition_data->start_addr, partition_data->start_addr + partition_data->total_size, NULL, "/", NULL, 0, NULL);

	if (!oem_device) {
		gxloge("this partition mount failed!\n");
		goto err;
	}
	fd = minifs_open(oem_device, file_name, 0, 0644);
	if (!fd) {
		gxloge("open error: %s %d\n", __func__, __LINE__);
		fd = minifs_open(oem_device, file_name, MINIFS_CREAT, 0644);
		if (!fd) {
			gxloge(" creat error: %s %d\n", __func__, __LINE__);
			goto err;
		}
	}
	return fd;
err:
	return NULL;
}
#endif

static unsigned int myhtoi(const char *str)
{
	const char *cp;
	unsigned int data = 0, bdata = 0;

	if (str == NULL)
		return 0;

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}
#endif

static int ini_open_oem(char* oem_path)
{
#ifdef CONFIG_ENABLE_MINIFS
	if (g_oem_use_minifs) {
#ifdef NO_OS
		oem_file = gxopen("DATA", oem_path);
#else
		oem_file = GxCore_Open(oem_path, "r+");
		if (-1 == oem_file) { //oem is not exit, need create
			oem_file = GxCore_Open(oem_path, "c");
			if(!oem_file)
				return -1;
		}
#endif
	}
	else
#endif
	{
		if (flash_partition_table == NULL)
			flash_partition_table = GxCore_PartitionFlashInit();
		if (flash_partition_table == NULL) {
			gxloge(" partition table init failed\n");
			return -1;
		}
		oem_partition = GxCore_PartitionGetByName(flash_partition_table, oem_path);
		if (oem_partition == NULL) {
			gxloge(" oem partition by name err\n");
			return -1;
		}
	}

	return 0;
}

static int ini_oem_size(void)
{
	int size = 0;
#ifdef CONFIG_ENABLE_MINIFS
	if (g_oem_use_minifs) {
#ifdef NO_OS
		size = minifs_size(oem_file);
#else
		GxFileInfo info;
		GxCore_GetFileStat(oem_file,&info);
		size = info.size_by_bytes;
#endif
	}
	else
#endif
	{
		char* buf = NULL;
		buf = GxCore_Malloc(ini_block_size);
		if (buf == NULL)
			return 0;
		GxCore_PartitionRead(oem_partition, buf, ini_block_size, 0);
		size = ini_calc_oem_patition_used_size(buf, ini_block_size);
		GxCore_Free(buf);
	}

	return size;
}

static int ini_read_oem(char* buf, int size)
{
	int read_size = 0;
#ifdef CONFIG_ENABLE_MINIFS
	if (g_oem_use_minifs) {
#ifdef NO_OS
		read_size = minifs_read(oem_file, buf, size, 0);
#else
		read_size = GxCore_ReadAt(oem_file, buf, 1, size, 0);
#endif
	}
	else
#endif
	{
		GxCore_PartitionRead(oem_partition, buf, size, 0);
		read_size = ini_calc_oem_patition_used_size(buf, size);
	}

	if (strncmp(buf, OEM_HEARDER, strlen(OEM_HEARDER)) != 0)
		read_size = 0;

	return read_size;
}

static int ini_write_oem(char* buf, int size)
{
#ifdef CONFIG_ENABLE_MINIFS
	if (g_oem_use_minifs) {
#ifdef NO_OS
		minifs_truncate(oem_file, 0);
		if (minifs_write(oem_file, buf, size, 0) <= 0)
			return -1;
		minifs_sync(oem_file);
#else
		GxCore_Truncate(oem_file, 0);
		if (GxCore_WriteAt(oem_file, buf, 1, size, 0) <= 0)
			return -1;
		GxCore_Sync(oem_file);
#endif
	}
	else
#endif
	{
		if (ini_write_oem_partition(oem_partition, buf, size) < 0)
			return -1;
	}

	return 0;
}

static int ini_open_backoem(char* backoem_path)
{
#ifdef CONFIG_ENABLE_MINIFS
	if (g_backoem_use_minifs) {
#ifdef NO_OS
		backoem_file = gxopen("DATA", backoem_path);
#else
		backoem_file = GxCore_Open(backoem_path, "r+");
		if (-1 == backoem_file) { //backoem is not exit, need create
			backoem_file = GxCore_Open(backoem_path, "c");
			if(!backoem_file)
				return -1;
		}
#endif
	}
	else
#endif
	{
		if (flash_partition_table == NULL)
			flash_partition_table = GxCore_PartitionFlashInit();
		if (flash_partition_table == NULL) {
			gxloge(" partition table init failed\n");
			return -1;
		}
		backoem_partition = GxCore_PartitionGetByName(flash_partition_table, backoem_path);
		if (backoem_partition == NULL) {
			gxloge("backoem partition by name err\n");
			return -1;
		}
	}

	return 0;
}

static int ini_backoem_size(void)
{
	int size = 0;
#ifdef CONFIG_ENABLE_MINIFS
	if (g_backoem_use_minifs) {
#ifdef NO_OS
		size = minifs_size(backoem_file);
#else
		GxFileInfo info;
		GxCore_GetFileStat(backoem_file,&info);
		size = info.size_by_bytes;
#endif
	}
	else
#endif
	{
		char* buf = NULL;
		buf = GxCore_Malloc(ini_block_size);
		if (buf == NULL)
			return 0;
		GxCore_PartitionRead(backoem_partition, buf, ini_block_size, 0);
		size = ini_calc_oem_patition_used_size(buf, ini_block_size);
		GxCore_Free(buf);
	}

	return size;
}

static int ini_read_backoem(char* buf, int size)
{
	int read_size = 0;
#ifdef CONFIG_ENABLE_MINIFS
	if (g_backoem_use_minifs) {
#ifdef NO_OS
		read_size = minifs_read(backoem_file, buf, size, 0);
#else
		read_size = GxCore_ReadAt(backoem_file, buf, 1, size, 0);
#endif
	}
	else
#endif
	{
		GxCore_PartitionRead(backoem_partition, buf, size, 0);
		read_size = ini_calc_oem_patition_used_size(buf, size);
	}

	if (strncmp(buf, OEM_HEARDER, strlen(OEM_HEARDER)) != 0)
		read_size = 0;

	return read_size;
}

static int ini_write_backoem(char* buf, int size)
{
#ifdef CONFIG_ENABLE_MINIFS
	if (g_backoem_use_minifs) {
#ifdef NO_OS
		minifs_truncate(backoem_file, 0);
		if (minifs_write(backoem_file, buf, size, 0) <= 0)
			return -1;
		minifs_sync(backoem_file);
#else
		GxCore_Truncate(backoem_file, 0);
		if (GxCore_WriteAt(backoem_file, buf, 1, size, 0) <= 0)
			return -1;
		GxCore_Sync(backoem_file);
#endif
	}
	else
#endif
	{
		if (ini_write_oem_partition(backoem_partition, buf, size) < 0)
			return -1;
	}

	return 0;
}

static int32_t ini_setcrcvalue(uint32_t crc32, uint32_t crc32_back)
{
	int ret = 0;
#ifdef CONFIG_ENABLE_MINIFS
	int size = 0;
	int file_size = 0;
	char oemvalue[10];
#endif
	char *buf = NULL;

	if (crc32 != 0) {
#ifdef CONFIG_ENABLE_MINIFS
		if (g_oem_use_minifs) {
			memset(oemvalue, 0, 10);
			sprintf((char*)oemvalue, "%02x%02x%02x%02x",
					(crc32 >> 24) & 0xff, (crc32 >> 16) & 0xff, (crc32 >> 8) & 0xff, crc32 & 0xff);
			ret = ini_set(c_ini, "OEM", "CRC32", oemvalue);
			if (ret < 0) {
				gxloge("c_ini do not have the section and parameter, append it to the b_ini!\n");
				goto err;
			}
#ifdef NO_OS
			size = minifs_size(crc_file);
			buf = (char *)GxCore_Malloc(2*size);
			if (buf == NULL) {
				goto err;
			}

			memset(buf, 0, 2*size);
			file_size = ini_savememory(c_ini, buf, 2*size);
			if (file_size > 0) {
				ret = minifs_write(crc_file, buf, file_size, 0);
				if (ret <= 0) {
					gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
					goto err;
				}
				minifs_sync(crc_file);
			}
#else
			GxFileInfo info;
			GxCore_GetFileStat(crc_file,&info);
			size = info.size_by_bytes;
			buf = (char *)GxCore_Malloc(2*size);
			if (buf == NULL) {
				goto err;
			}

			memset(buf, 0, 2*size);
			file_size = ini_savememory(c_ini, buf, 2*size);
			if (file_size > 0) {
				ret = GxCore_WriteAt(crc_file, buf, 2, file_size, 0);
				if (ret <= 0) {
					gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
					goto err;
				}
				GxCore_Sync(crc_file);
			}
#endif
			if (buf != NULL) {
				GxCore_Free(buf);
				buf = NULL;
			}
		}
		else
#endif
		{
			buf = GxCore_Malloc(ini_block_size);
			if (buf == NULL) {
				ret = -1;
				goto err;
			}
			GxCore_PartitionRead(oem_partition, buf, ini_block_size, 0);
			memcpy(&buf[ini_block_size - 4], &crc32, 4);
			GxCore_PartitionErase(oem_partition);
			if (GxCore_PartitionWriteRaw(oem_partition, buf, ini_block_size, 0) != 1) {
				ret = -1;
				goto err;
			}
		}
	}

	if (crc32_back != 0) {
#ifdef CONFIG_ENABLE_MINIFS
		if (g_backoem_use_minifs) {
			memset(oemvalue, 0, 10);
			sprintf((char*)oemvalue, "%02x%02x%02x%02x",
					(crc32_back >> 24) & 0xff, (crc32_back >> 16) & 0xff, (crc32_back >> 8) & 0xff, crc32_back & 0xff);
			ret = ini_set(c_ini, "BACK_OEM", "CRC32", oemvalue);
			if (ret < 0) {
				gxloge("c_ini do not have the section and parameter, append it to the b_ini!\n");
				goto err;
			}
#ifdef NO_OS
			size = minifs_size(crc_file);
			buf = (char *)GxCore_Malloc(2*size);
			if (buf == NULL) {
				goto err;
			}

			memset(buf, 0, 2*size);
			file_size = ini_savememory(c_ini, buf, 2*size);
			if (file_size > 0) {
				ret = minifs_write(crc_file, buf, file_size, 0);
				if (ret <= 0) {
					gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
					goto err;
				}
				minifs_sync(crc_file);
			}
#else
			GxFileInfo info;
			GxCore_GetFileStat(crc_file,&info);
			size = info.size_by_bytes;
			buf = (char *)GxCore_Malloc(2*size);
			if (buf == NULL) {
				goto err;
			}

			memset(buf, 0, 2*size);
			file_size = ini_savememory(c_ini, buf, 2*size);
			if (file_size > 0) {
				ret = GxCore_WriteAt(crc_file, buf, 2, file_size, 0);
				if (ret <= 0) {
					gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
					goto err;
				}
				GxCore_Sync(crc_file);
			}
#endif
			if (buf != NULL) {
				GxCore_Free(buf);
				buf = NULL;
			}
		}
		else
#endif
		{
			buf = GxCore_Malloc(ini_block_size);
			if (buf == NULL) {
				ret = -1;
				goto err;
			}
			GxCore_PartitionRead(backoem_partition, buf, ini_block_size, 0);
			memcpy(&buf[ini_block_size - 4], &crc32_back, 4);
			GxCore_PartitionErase(backoem_partition);
			if (GxCore_PartitionWriteRaw(backoem_partition, buf, ini_block_size, 0) != 1) {
				ret = -1;
				goto err;
			}
		}
	}

err:
	if (buf != NULL) {
		GxCore_Free(buf);
		buf = NULL;
	}
	return ret;
	
}

static int32_t ini_save_oem(char* buf, int32_t size)
{
	int32_t ret = -1;
	uint32_t crc_result = 0;
	int32_t file_size = 0;

	if (v_ini != 0)
	{
		if(buf != NULL)
		{
			if (ini_write_oem(buf, size) != 0) {
				gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
				goto err;
			}

			if (g_oem_need_backup) {
				file_size = ini_oem_size();
				file_size = ini_read_oem(buf, file_size);

				crc_result = GxCore_Crc32(0, (uint8_t *)buf, file_size);
				ini_setcrcvalue(crc_result, 0);

				if (ini_write_backoem(buf, size) != 0) {
					gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
					goto err;
				}

				ini_setcrcvalue(0, crc_result);
			}
		}
	}

	ret = 0;
err:
	return ret;
}
int32_t Ini_ReadOld(char *buf, int32_t *size)
{
	int ret = 0;

	if (buf == NULL) {
		goto err;
	}

	GxCore_MutexLock(ini_mutex);
	if(v_ini != 0)
	{
		*size = ini_savememory(v_ini, buf, ini_block_size);
		if (*size <= 0) {
			gxloge("v_ini memory size zero!\n");
			goto err;
		}
	}
err:
	GxCore_MutexUnlock(ini_mutex);
	return ret;

}
int32_t Ini_WriteOld2New(char *buf, int32_t size)
{
	int ret = 0;

	if (buf == NULL || size <= 0) {
		goto err;
	}

	GxCore_MutexLock(ini_mutex);
	if(v_ini != 0)
	{
		ret = ini_save_oem(buf, size);
		v_ini = ini_loadmemory(buf, size);
	}
err:
	GxCore_MutexUnlock(ini_mutex);
	return ret;
}

char *Ini_GetValue(const char *section, const char *parameter)
{
	char *value = NULL;

	ASSERT(section != NULL);
	ASSERT(parameter != NULL);
	if (section == NULL || parameter == NULL) {
		return NULL;
	}
	GxCore_MutexLock(ini_mutex);
	value = ini_get(v_ini, section, parameter);
	GxCore_MutexUnlock(ini_mutex);

	return value;
}

int32_t Ini_SetValue(const char *section, const char *parameter, const char *value)
{
	int ret = 0;
	char *tmp;

	ASSERT(section != NULL);
	ASSERT(parameter != NULL);
	ASSERT(value != NULL);

	if (section == NULL || parameter == NULL) {
		goto err;
	}
	GxCore_MutexLock(ini_mutex);
	if(v_ini != 0)
	{
		tmp = ini_get(v_ini, section, parameter);
		if (tmp == NULL) {
			gxloge("I_ini could not be channed!\n");
			ret = -1;
			goto err;
		}

		ret = ini_set(v_ini, section, parameter, value);

		if (ret < 0) {
			gxloge("b_ini do not have the section and parameter, append it to the b_ini!\n");
			goto err;
		}
	}
err:
	GxCore_MutexUnlock(ini_mutex);
	return ret;
}

int32_t Ini_Sync(void)
{
	int ret = 0;
	char *buf = NULL;
	int32_t size = 0;

	GxCore_MutexLock(ini_mutex);
	if(v_ini != 0) {
		buf = (char *)GxCore_Malloc(ini_block_size);
		if (buf == NULL) {
			goto err;
		}
		memset(buf, 0, ini_block_size);
		size = ini_savememory(v_ini, buf, ini_block_size);
		if (size <= 0) {
			gxloge("v_ini memory size zero!\n");
			goto err;
		}
		ret = ini_save_oem(buf, size);
	}
err:
	if (buf != NULL) {
		GxCore_Free(buf);
		buf = NULL;
	}
	GxCore_MutexUnlock(ini_mutex);

	return ret;
}

static int ini_load_crc(char* crc_file_path)
{
#ifdef CONFIG_ENABLE_MINIFS
	if (g_oem_use_minifs || g_backoem_use_minifs) {
		int size = 0;
		int crc_file_size = 0;
		char* buf = NULL;
#ifdef NO_OS
		crc_file = gxopen("DATA", crc_file_path);
		size = minifs_size(crc_file);
		if (size == 0) {
			if (minifs_write(crc_file, MINIFS_CRC_FILE_INITIAL_VALUE, strlen(MINIFS_CRC_FILE_INITIAL_VALUE), 0) <= 0)
				goto err;
			minifs_sync(crc_file);
			size = minifs_size(crc_file);
		}
		buf = GxCore_Malloc(size);
		if(buf == NULL)
			goto err;
		memset(buf, 0, size);
		crc_file_size = minifs_read(crc_file, buf, size, 0);
		if (crc_file_size != size ) {
			gxloge("crc_file_size = %d\n", crc_file_size);
			gxloge("error: %s %d\n", __func__, __LINE__);
			goto err;
		}
		c_ini = ini_loadmemory(buf, crc_file_size);
#else
		crc_file = GxCore_Open(crc_file_path, "r+");
		if (-1 == crc_file) { //crc file not exit, need create
			gxloge("error: %s %d\n", __func__, __LINE__);
			crc_file = GxCore_Open(crc_file_path, "c");
			if(!crc_file)
				goto err;
		}
		GxFileInfo info;
		GxCore_GetFileStat(crc_file,&info);
		size = info.size_by_bytes;
		if (size == 0) {
			if (GxCore_WriteAt(crc_file, MINIFS_CRC_FILE_INITIAL_VALUE, 1, strlen(MINIFS_CRC_FILE_INITIAL_VALUE), 0) <= 0) {
				gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
				goto err;
			}
			GxCore_Sync(crc_file);
			GxCore_GetFileStat(crc_file,&info);
			size = info.size_by_bytes;
		}
		buf = GxCore_Malloc(size);
		if(buf == NULL)
			goto err;
		memset(buf, 0, size);
		crc_file_size = GxCore_ReadAt(crc_file, buf, 1, size, 0);
		if (crc_file_size != size ) {
			GxCore_Close(crc_file);
			gxloge("crc_file_size = %d\n", crc_file_size);
			gxloge("error: %s %d\n", __func__, __LINE__);
			goto err;
		}
		c_ini = ini_loadmemory(buf, crc_file_size);
#endif
		if (buf)
		{
			GxCore_Free(buf);
			buf = NULL;
		}
		return 0;
	err:
		if (buf)
		{
			GxCore_Free(buf);
			buf = NULL;
		}
		return -1;
	}
	else
#endif
	{
		//do nothing
	}

	return 0;
}

int32_t Ini_oem_use_minifs(void)
{
#ifdef CONFIG_ENABLE_MINIFS
	return g_oem_use_minifs;
#else
	return 0;
#endif
}

const char *Ini_oem_name(void)
{
	return (const char *)oem_file_name;
}

const char *Ini_backoem_name(void)
{
	return (const char *)backoem_file_name;
}

int32_t Ini_Oem_Init(char *dlp,char *backup_dlp)
{
#define MINIFS_CRC_FILE "/crc.ini"
#define MINIFS_CRC_FILE_NAME_LENGTH (40 + sizeof(MINIFS_CRC_FILE))
#define MINIFS_OEM_FILE_PATH_PREFIX "/home/gx"
	char *b_buf = NULL;
	char *v_buf = NULL;
	int32_t size = 0;
	int32_t file_size = 0;
	int32_t backfile_size = 0;
	uint32_t crc_result_back = 0;
	uint32_t crc_result_orig = 0;
	uint32_t crc_oem = 0;
	uint32_t crc_backoem = 0;
	char minifs_crc_oem_file[MINIFS_CRC_FILE_NAME_LENGTH+1] = {0};

#ifdef CONFIG_ENABLE_MINIFS
	char *tmp = NULL;
	char *backtmp = NULL;
	char *p_str = NULL;
#endif
	if (v_ini != 0)
		return -1;
	GxCore_MutexCreate(&ini_mutex);
	GxCore_MutexLock(ini_mutex);

	GxCore_PartitionGetFlashBlockSize(&ini_block_size);

	if (dlp == NULL || strlen(dlp) == 0) {
		gxloge("oem name is NULL\n");
		goto err;
	}

	if ((dlp != NULL && backup_dlp != NULL)
		&& (strcmp(dlp, backup_dlp) == 0)) {/* The name of dlp and backup_dlp must not be null and not be same */
		gxloge("oem and backoem must have different names\n");
		goto err;
	}

	if (backup_dlp == NULL || strlen(backup_dlp) == 0)
		g_oem_need_backup = 0;

	/* format oem file path when use minifs */
#ifdef CONFIG_ENABLE_MINIFS
	if (!strncmp(dlp, "/", 1)) {
		if (strlen(dlp) > (MINIFS_CRC_FILE_NAME_LENGTH - sizeof(MINIFS_CRC_FILE)))
			goto err;
		g_oem_use_minifs = true;
		p_str = dlp; //for crc file path
#ifdef NO_OS
		/* nos去掉路径名的/home/gx */
		dlp = dlp + strlen(MINIFS_OEM_FILE_PATH_PREFIX);
#endif
	}

#endif

	/* format backoem file path when use minifs */
	if (g_oem_need_backup) {
#ifdef CONFIG_ENABLE_MINIFS
		if (!strncmp(backup_dlp, "/", 1)) {
			if (strlen(backup_dlp) > (MINIFS_CRC_FILE_NAME_LENGTH - sizeof(MINIFS_CRC_FILE)))
				goto err;
			g_backoem_use_minifs = true;
			p_str = backup_dlp; //for crc file path
#ifdef NO_OS
			/* nos去掉路径名的/home/gx */
			backup_dlp = backup_dlp + strlen(MINIFS_OEM_FILE_PATH_PREFIX);
#endif
		}

#endif

		/* format crc file path when use minifs */
#ifdef CONFIG_ENABLE_MINIFS
		if (g_oem_use_minifs || g_backoem_use_minifs) {
			memcpy(minifs_crc_oem_file, p_str, strlen(p_str));

			p_str = strrchr(minifs_crc_oem_file, '/');
			if (p_str == NULL)
				goto err;

			memcpy(p_str, MINIFS_CRC_FILE, sizeof(MINIFS_CRC_FILE)); //这里用sizeof是为了拷贝'\0'过去
#ifdef NO_OS
			/* nos去掉路径名的/home/gx */
			memmove(minifs_crc_oem_file, minifs_crc_oem_file+strlen(MINIFS_OEM_FILE_PATH_PREFIX), strlen(p_str));
			minifs_crc_oem_file[strlen(MINIFS_CRC_FILE)] = '\0';
#endif
		}
#endif
	}

	/* load oem */
	if (ini_open_oem(dlp) != 0) {
		gxloge("open oem err !!!\n");
		goto err;
	}
	size = ini_oem_size();
	if (size > 0) {
		v_buf = GxCore_Malloc(size);
		if(v_buf == NULL)
			goto err;
		memset(v_buf, 0, size);
		file_size = ini_read_oem(v_buf, size);

		crc_result_orig = GxCore_Crc32(0, (uint8_t *)v_buf, file_size);
		v_ini = ini_loadmemory(v_buf, file_size);
	}

	/* read back oem to compare oem */
	if (g_oem_need_backup) {
		if (ini_open_backoem(backup_dlp) != 0) {
			gxloge("open back oem err !!!\n");
			goto err;
		}
		size = ini_backoem_size();
		if (size > 0) {
			b_buf = GxCore_Malloc(size);
			if(b_buf == NULL)
				goto err;
			memset(b_buf, 0, size);

			backfile_size = ini_read_backoem(b_buf, size);

			crc_result_back = GxCore_Crc32(0, (uint8_t *)b_buf, backfile_size);
		}
	}

	if (file_size == 0 && backfile_size == 0) {
		gxloge("oem file is empty\n");
		goto err;
	}

	if (g_oem_need_backup) {
		if (ini_load_crc(minifs_crc_oem_file) != 0)
			goto err;

		/* get and check crc */
#ifdef CONFIG_ENABLE_MINIFS
		if (g_oem_use_minifs) {
			tmp = ini_get(c_ini, "OEM", "CRC32");
			crc_oem = myhtoi(tmp);
		}
		else
#endif
		{
			/* oem和backoem分区的CRC校验放在各自的分区的最后的4个字节*/
			GxCore_PartitionRead(oem_partition, &crc_oem, 4, ini_block_size - 4);
		}

#ifdef CONFIG_ENABLE_MINIFS
		if (g_backoem_use_minifs) {
			backtmp = ini_get(c_ini, "BACK_OEM", "CRC32");
			crc_backoem = myhtoi(backtmp);
		}
		else
#endif
		{
			/* oem和backoem分区的CRC校验放在各自的分区的最后的4个字节*/
			GxCore_PartitionRead(backoem_partition, &crc_backoem, 4, ini_block_size - 4);
		}

		gxlogd("crc_oem : 0x%08x\n", crc_oem);
		gxlogd("crc_backoem : 0x%08x\n", crc_backoem);
		gxlogd("crc_result_orig : 0x%08x\n", crc_result_orig);
		gxlogd("crc_result_back : 0x%08x\n", crc_result_back);

		if (file_size == 0 && backfile_size != 0) {
			if (ini_write_oem(b_buf, backfile_size) != 0) {
				gxloge("check write original err\n");
				goto err;
			}

			if (crc_result_back == crc_backoem)
				ini_setcrcvalue(crc_result_back, 0);
			else
				ini_setcrcvalue(crc_result_back, crc_result_back);
			v_ini = ini_loadmemory(b_buf, backfile_size);
		} else if ((crc_oem == 0xFFFFFFFF && crc_backoem == 0xFFFFFFFF)
				|| (file_size != 0 && backfile_size == 0)) {
			if (ini_write_backoem(v_buf, file_size) != 0) {
				gxloge("%s,%d, ini write error!\n", __func__, __LINE__);
				goto err;
			}
			ini_setcrcvalue(crc_result_orig, crc_result_orig);
		} else {
			if (crc_result_orig == crc_oem) {
				if (crc_result_back == crc_backoem) {
					if (crc_oem != crc_backoem) {
						if (ini_write_backoem(v_buf, file_size) != 0) {
							gxloge("check write backup err\n");
							goto err;
						}

						ini_setcrcvalue(0, crc_result_orig);
					}
				} else {
					if (ini_write_backoem(v_buf, file_size) != 0) {
						gxloge("check write backup err\n");
						goto err;
					}

					ini_setcrcvalue(0, crc_result_orig);
				}
			} else {
				if (ini_write_oem(b_buf, backfile_size) != 0) {
					gxloge("check write original err\n");
					goto err;
				}

				if (crc_result_back == crc_backoem)
					ini_setcrcvalue(crc_result_back, 0);
				else
					ini_setcrcvalue(crc_result_back, crc_result_back);
				v_ini = ini_loadmemory(b_buf, backfile_size);
			}
		}
	}

	memset(oem_file_name, 0x00, sizeof(oem_file_name));
	memset(backoem_file_name, 0x00, sizeof(backoem_file_name));
	if (dlp != NULL) {
		strncpy(oem_file_name, dlp, MAX_OEM_FILE_NAME_LEN);
	}
	if (backup_dlp != NULL) {
		strncpy(backoem_file_name, backup_dlp, MAX_OEM_FILE_NAME_LEN);
	}

	if (b_buf)
	{
		GxCore_Free(b_buf);
		b_buf = NULL;
	}
	if (v_buf)
	{
		GxCore_Free(v_buf);
		v_buf = NULL;
	}
	GxCore_MutexUnlock(ini_mutex);
	return 0;
err:
	if (b_buf)
	{
		GxCore_Free(b_buf);
		b_buf = NULL;
	}
	if (v_buf)
	{
		GxCore_Free(v_buf);
		v_buf = NULL;
	}
	GxCore_MutexUnlock(ini_mutex);
	return -1;
}

int32_t Ini_Oem_Close()
{
	ini_close(v_ini);
	v_ini = 0;
#ifdef CONFIG_ENABLE_MINIFS
	ini_close(c_ini);
	if (g_oem_use_minifs || g_backoem_use_minifs) {
#ifdef NO_OS
		minifs_umount(oem_device);
		oem_device = NULL;
#else
		GxCore_Close(oem_file);
		if (g_oem_need_backup) {
			GxCore_Close(backoem_file);
			GxCore_Close(crc_file);
		}
#endif
	}
#endif

#ifdef CONFIG_ENABLE_MINIFS
#ifdef NO_OS
	oem_device = NULL;
	oem_file = NULL;
	backoem_file = NULL;
	crc_file = NULL;
#else
	oem_file = 0;
	backoem_file = 0;
	crc_file = 0;
#endif
	c_ini = 0;
	g_oem_use_minifs = false;
	g_backoem_use_minifs = false;
#endif /*CONFIG_ENABLE_MINIFS*/
	flash_partition_table = NULL;
	oem_partition = NULL;
	backoem_partition = NULL;

	GxCore_MutexDelete(ini_mutex);

	return 0;
}
