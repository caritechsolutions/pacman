/**
 * @file    gxconfig.c
 * @author  lixb
 * @date    20100127
 * @brief   The tool of configure STB information
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common/libini.h"
#include "gxcore.h"
#include "module/config/gxconfig.h"
#ifdef NO_OS
#include "io.h"
#endif /*NO_OS*/

#define GXBUS_CONFIG_LOCK(ptr)          GxCore_MutexLock((ptr)->lock)
#define GXBUS_CONFIG_UNLOCK(ptr)        GxCore_MutexUnlock((ptr)->lock)

#define INI_SECT_LEN    64
#define INI_PARM_LEN    64
#define INI_VALUE_LEN   192

struct config_info {
	handle_t    ini;
	handle_t    lock;
};

struct config_group {
	bool        changed_flag;
	handle_t    virtual_ini;
	handle_t    lock;
};

static struct config_info gxbus_config = {
	EINVALID_HANDLE,
	EINVALID_HANDLE
};

static char* gxbus_config_save(handle_t ini)
{
#define FILE_READ_BLOCK_SIZE        (512)
	char*       ret;
	uint32_t   fp;
	uint32_t   backup_fp;
	char       buf[FILE_READ_BLOCK_SIZE];
	int        len;

	ini_remove(ini, GXBUS_FILE_PROTECT, NULL);
	ini_append(ini, GXBUS_FILE_PROTECT, GXBUS_FILE_PROTECT,GXBUS_FILE_PROTECT);
	ret = ini_save(ini);

	backup_fp = GxCore_Open((const char*)GXBUS_CONFIG_FILE_BACKUP, "c");
	fp = GxCore_Open(GXBUS_CONFIG_FILE_NAME, "c");
	if (fp == -1 || backup_fp == -1) {
		return NULL;
	}
	//gxlogd("\n%s\n", GXBUS_CONFIG_FILE_BACKUP);
	do {
		len = GxCore_Read(fp,buf,1,FILE_READ_BLOCK_SIZE);
		GxCore_Write(backup_fp,buf,1,len);
		//gxlogd("%s\n", buf);
	}while(len >= FILE_READ_BLOCK_SIZE);
	GxCore_Close(fp);
	GxCore_Sync(backup_fp);
	GxCore_Close(backup_fp);

	return ret;
}

#ifdef MULTIPROCESS
static void gxbus_config_close(void)
{
	if (gxbus_config.lock == EINVALID_HANDLE) {
		GxCore_MutexCreate(&gxbus_config.lock);
	}

	GXBUS_CONFIG_LOCK(&gxbus_config);
	if (gxbus_config.ini != EINVALID_HANDLE) {
		ini_close(gxbus_config.ini);
		gxbus_config.ini = EINVALID_HANDLE;
	}
	GXBUS_CONFIG_UNLOCK(&gxbus_config);
}
#else
#define gxbus_config_close() ((void)0)
#endif

static struct config_info* gxbus_config_open(void)
{
	if (gxbus_config.lock == EINVALID_HANDLE) {
		GxCore_MutexCreate(&gxbus_config.lock);
	}

	GXBUS_CONFIG_LOCK(&gxbus_config);
	if (gxbus_config.ini == EINVALID_HANDLE) {
		handle_t    ini;
		char*       str;

		/*It have not initilized.*/
		gxbus_config.ini = ini_open(GXBUS_CONFIG_FILE_NAME);
		if (gxbus_config.ini != EINVALID_HANDLE) {
			/*ini file is existent*/
			str = ini_get(gxbus_config.ini, GXBUS_FILE_PROTECT, GXBUS_FILE_PROTECT);
			if (str != NULL && strncmp(str, GXBUS_FILE_PROTECT, 4) == 0) {
				/*ini file is ok*/
				GXBUS_CONFIG_UNLOCK(&gxbus_config);
				return &gxbus_config;
			}
			/*ini file have been damaged*/
		} else {
			/*ini file is not existent. create it now!*/
			gxbus_config.ini = ini_create(GXBUS_CONFIG_FILE_NAME);
		}
		ini = ini_open(GXBUS_CONFIG_FILE_BACKUP);
		if (gxbus_config.ini != EINVALID_HANDLE) {
			/*restore the ini file from backup file*/
			gxbus_config_save(ini_merge(gxbus_config.ini,ini));
		}
		ini_close(ini);
	}
	GXBUS_CONFIG_UNLOCK(&gxbus_config);

	return &gxbus_config;
}

static char* get_section(const char* key, char* section, size_t size)
{
	int i = 0;

	assert(key != NULL);
	assert(section != NULL);

	if (key == NULL || section == NULL) {
		return NULL;
	}

	while(key[i] != KEY_SEPARATOR && key[i] != '\0' && i < size) {
		section[i] = key[i];
		i++;
	}

	if (i <size) {
		section[i] = '\0';
	} else {
		return NULL;
	}

	return section;
}

static char* get_param(const char* key, char* param, size_t size)
{
	int   len;
	char* s = (char*)key;

	assert(key != NULL);
	assert(param != NULL);

	if (key == NULL || param == NULL) {
		return NULL;
	}

	while(*s != KEY_SEPARATOR && *s != '\0') {
		s++;
	}

	if (*s++ != KEY_SEPARATOR) {
		s = (char*)key;
	}

	len = strlen(s);
	if (len >= size) {
		return NULL;
	}

	return strncpy(param, s, size);
}

/**
 * @brief       从配置表中删除key标识的变量
 * @param [in]  key         变量标识字符串
 * @return      int32_t     函数执行成功，返回0，错误返回-1
 * @see         ::GxBus_ConfigSet
 * @remarks     如果key包含section和param域，则删除这个变量，如果只有section域
 则删除这个域的所有变量，此函数为内部函数，暂只提供给测试用，不
 会封装给用户
 */
int32_t GxBus_ConfigRemove(const char* key)
{
	int32_t             ret;
	struct config_info* config;
	char                section[INI_SECT_LEN];
	char                param[INI_PARM_LEN];

	assert(key != NULL);

	if (key == NULL) {
		return -1;
	}

	config = gxbus_config_open();

	ret = ini_remove(config->ini,
			get_section(key, section, INI_SECT_LEN),
			get_param(key, param, INI_PARM_LEN));

	gxbus_config_close();
	return ret;
}

void GxBus_ConfigLoadDefault(void)
{
	if (gxbus_config.lock == EINVALID_HANDLE) {
		GxCore_MutexCreate(&gxbus_config.lock);
	}

	GXBUS_CONFIG_LOCK(&gxbus_config);
	if (gxbus_config.ini != EINVALID_HANDLE) {
		ini_close(gxbus_config.ini);
		gxbus_config.ini = EINVALID_HANDLE;
	}
	GxCore_FileDelete(GXBUS_CONFIG_FILE_NAME);
	GxCore_FileDelete(GXBUS_CONFIG_FILE_BACKUP);
	GXBUS_CONFIG_UNLOCK(&gxbus_config);
}

#ifdef NO_OS
#define GX_CONFIG_PARTITION_SIZE 64 * 1024
static char mem_config[GX_CONFIG_PARTITION_SIZE] = {0};
#endif /*NO_OS*/

static void* init_config_file(int *size)
{
	void *mem = NULL;
#ifdef NO_OS
	#define GX_CONFIG_PARTITION                 "CONFIG"
	struct partition *flash_table = NULL;
	struct partition_info *partition = NULL;
	int ret = 0;

	flash_table = GxCore_PartitionFlashInit();
	if(NULL == flash_table) {
		return (NULL);
	}

	partition = GxCore_PartitionGetByName((struct partition *)flash_table, GX_CONFIG_PARTITION);
	if(NULL == partition) {
		return (NULL);
	}

	mem = mem_config;
	if(NULL == mem) {
		return (NULL);
	}

	ret = GxCore_PartitionRead(partition, mem, GX_CONFIG_PARTITION_SIZE, 0);
	if(ret <= 0) {
		GxCore_Free(mem);
		return (NULL);
	}

	file_add(GXBUS_CONFIG_FILE_NAME, mem, GX_CONFIG_PARTITION_SIZE);
	*size = GX_CONFIG_PARTITION_SIZE;
#endif /*NO_OS*/
	return (mem);
}

static void save_config_file(void *mem, int size)
{
#ifdef NO_OS
	struct partition *flash_table = NULL;
	struct partition_info *partition = NULL;
	int ret = 0;
	uint32_t fp = 0;

	if((NULL == mem) || (0 == size)) {
		return;
	}

	fp = GxCore_Open(GXBUS_CONFIG_FILE_NAME, "r");
	if(0 == fp) {
		return;
	}

	ret = GxCore_Read(fp, mem, size, 1);
	if(ret <= 0) {
		return;
	}

	GxCore_Close(fp);

	flash_table = GxCore_PartitionFlashInit();
	if(NULL == flash_table) {
		return;
	}

	partition = GxCore_PartitionGetByName((struct partition *)flash_table, GX_CONFIG_PARTITION);
	if(NULL == partition) {
		return;
	}

	ret = GxCore_PartitionWrite(partition, mem, size, 0);
	if(ret <= 0) {
		return;
	}
#endif /*NO_OS*/
}

static void release_config_file(void *mem)
{
#ifdef NO_OS
	if(mem) {
		file_del(GXBUS_CONFIG_FILE_NAME);
	}
#endif /*NO_OS*/
}

char* __attribute__((weak)) GxBus_ConfigGet(const char* key,
		char*       buf,
		size_t      buf_size,
		const char* dvalue)
{
	char*               ret;
	char*               value;
	void*               config_mem = NULL;
	int                 config_size = 0;
	struct config_info* config;
	char                section[INI_SECT_LEN];
	char                param[INI_PARM_LEN];

	assert(key != NULL);
	assert(buf != NULL);
	assert(dvalue != NULL);

	if (key == NULL || buf == NULL || dvalue == NULL) {
		return NULL;
	}

	config_mem = init_config_file(&config_size);

	config = gxbus_config_open();

	if (get_section(key, section, INI_SECT_LEN) == NULL
			|| get_param(key, param, INI_PARM_LEN) == NULL) {
		release_config_file(config_mem);
		return NULL;
	}

	GXBUS_CONFIG_LOCK(config);
	value = ini_get(config->ini, section, param);
	if ( value == NULL) {
		/*默认值不需要存储，提高config模块访问效率*/
		//ini_append(config->ini, section, param, dvalue);
		//gxbus_config_save(config->ini);
		value = (char*)dvalue;
	}
	GXBUS_CONFIG_UNLOCK(config);

	ret = strncpy(buf, value, buf_size);
	gxbus_config_close();

	release_config_file(config_mem);

	return ret;
}

int32_t*  __attribute__((weak)) GxBus_ConfigGetInt(const char* key, int32_t* buf, int32_t dvalue)
{
	char str[INI_VALUE_LEN];
	char def[INI_VALUE_LEN];

	assert(key != NULL);
	assert(buf != NULL);

	snprintf(def, INI_VALUE_LEN, "%d", dvalue);

	if (GxBus_ConfigGet(key, str, INI_VALUE_LEN, def) != NULL) {
		*buf = atoi(str);
		return buf;
	}

	return NULL;
}

void  __attribute__((weak)) GxBus_ConfigMutexInit(void)
{
	if (gxbus_config.lock == EINVALID_HANDLE) {
		GxCore_MutexCreate(&gxbus_config.lock);
	}
}

#ifndef NO_OS
double* GxBus_ConfigGetDouble(const char* key, double* buf, double dvalue)
{
	char str[INI_VALUE_LEN];
	char def[INI_VALUE_LEN];

	assert(key != NULL);
	assert(buf != NULL);

	snprintf(def, INI_VALUE_LEN, "%f", dvalue);

	if (GxBus_ConfigGet(key, str, INI_VALUE_LEN, def) != NULL) {
		*buf = atof(str);
		return buf;
	}

	return NULL;
}
#endif

int32_t  __attribute__((weak)) GxBus_ConfigSet(const char* key, const char* value)
{
	struct config_info* config;
	void*               config_mem = NULL;
	int                 config_size = 0;
	char                section[INI_SECT_LEN];
	char                param[INI_PARM_LEN];
	int                 ret = -1;
	char                *value_old;

	assert(key != NULL);
	assert(value != NULL);

	if (key == NULL || value == NULL) {
		return EINPUT;
	}

	config_mem = init_config_file(&config_size);

	config = gxbus_config_open();

	if (get_section(key, section, INI_SECT_LEN) == NULL
			|| get_param(key, param, INI_PARM_LEN) == NULL) {
		release_config_file(config_mem);
		return GXCONFIG_FAILURE;
	}

	GXBUS_CONFIG_LOCK(config);
	value_old = ini_get(config->ini, section, param);
	if (value_old != NULL && strcmp(value, value_old) == 0) {
		release_config_file(config_mem);
		GXBUS_CONFIG_UNLOCK(config);
		return GXCONFIG_OK;
	}
	ret = ini_set(config->ini, section, param, value);
	if(ret < 0) {
		ini_append(config->ini, section, param, value);
	}
	if(ret != 0)
	{
		gxbus_config_save(config->ini);
	}
	GXBUS_CONFIG_UNLOCK(config);

	gxbus_config_close();

	save_config_file(config_mem, config_size);
	release_config_file(config_mem);

	return GXCONFIG_OK;
}

int32_t  __attribute__((weak)) GxBus_ConfigSetInt(const char* key, int32_t value)
{
	char str[INI_VALUE_LEN];

	memset(str, 0, sizeof(str));
	snprintf(str, INI_VALUE_LEN, "%d", value);

	return GxBus_ConfigSet(key, str);
}

int32_t GxBus_ConfigSetDouble(const char* key, double value)
{
	char str[INI_VALUE_LEN];

	memset(str, 0, sizeof(str));
	snprintf(str, INI_VALUE_LEN, "%f", value);

	return GxBus_ConfigSet(key, str);
}

handle_t GxBus_ConfigGroupOpen(void)
{
	handle_t                ini;
	struct config_group*    group;

	ini = ini_create(NULL);
	if (ini == EINVALID_HANDLE) {
		return EINVALID_HANDLE;
	}

	group = GxCore_Calloc(1, sizeof(struct config_group));
	if (group == NULL) {
		return EINVALID_HANDLE;
	}

	GxCore_MutexCreate(&group->lock);

	group->virtual_ini = ini;
	group->changed_flag = FALSE;

	return (handle_t)group;
}


int32_t GxBus_ConfigGroupClose(handle_t group)
{
	struct config_group* gp = (struct config_group*)group;

	if (group == GXCONFIG_FAILURE || gp == NULL) {
		return EINPUT;
	}

	ini_close(gp->virtual_ini);
	GxCore_MutexDelete(gp->lock);
	GxCore_Free(gp);

	return GXCONFIG_OK;
}

int32_t GxBus_ConfigGroupSave(handle_t group)
{
	struct config_info*     config;
	struct config_group*    gp = (struct config_group*)group;

	if (group == GXCONFIG_FAILURE || gp == NULL) {
		return EINPUT;
	}

	config = gxbus_config_open();

	GXBUS_CONFIG_LOCK(config);
	GXBUS_CONFIG_LOCK(gp);
	if (gxbus_config_save(ini_merge(config->ini, gp->virtual_ini)) == NULL) {
		GXBUS_CONFIG_UNLOCK(config);
		GXBUS_CONFIG_UNLOCK(gp);
		return GXCONFIG_FAILURE;
	}
	gp->changed_flag = FALSE;
	GXBUS_CONFIG_UNLOCK(config);
	GXBUS_CONFIG_UNLOCK(gp);

	gxbus_config_close();
	return GXCONFIG_OK;
}

char* GxBus_ConfigGroupGet(handle_t group,
		const char* key,
		char*       buf,
		size_t      buf_size,
		const char* dvalue)
{
	char                    *ret;
	char                    *value;
	struct config_info*     config;
	struct config_group*    gp = (struct config_group*)group;
	char                    section[INI_SECT_LEN];
	char                    param[INI_PARM_LEN];

	assert(key != NULL);
	assert(buf != NULL);
	assert(dvalue != NULL);
	assert(gp != NULL);

	if (key == NULL || buf == NULL || dvalue == NULL
			||group == GXCONFIG_FAILURE || gp == NULL) {
		return NULL;
	}

	config = gxbus_config_open();

	if (get_section(key, section, INI_SECT_LEN) == NULL
			|| get_param(key, param, INI_PARM_LEN) == NULL) {
		return NULL;
	}

	GXBUS_CONFIG_LOCK(gp);
	value = ini_get(gp->virtual_ini, section, param);
	GXBUS_CONFIG_UNLOCK(gp);

	if (value == NULL) {
		GXBUS_CONFIG_LOCK(config);
		value = ini_get(config->ini, section, param);
		GXBUS_CONFIG_UNLOCK(config);
		if (value == NULL) {
			//GXBUS_CONFIG_LOCK(gp);
			//ini_append(gp->virtual_ini, section, param, dvalue);
			//gp->changed_flag = TRUE;
			//GXBUS_CONFIG_UNLOCK(gp);
			value = (char*)dvalue;
		}
	}

	ret = strncpy(buf, value, buf_size);
	gxbus_config_close();
	return ret;
}

int32_t* GxBus_ConfigGroupGetInt(handle_t group,
		const char* key,
		int32_t*    buf,
		int32_t     dvalue)
{
	char str[INI_VALUE_LEN];
	char def[INI_VALUE_LEN];

	assert(buf != NULL);
	assert(key != NULL);

	snprintf(def, INI_VALUE_LEN, "%d", dvalue);

	if (GxBus_ConfigGroupGet(group, key, str, INI_VALUE_LEN, def) != NULL) {
		*buf = atoi(str);
		return buf;
	}

	return NULL;
}

#ifndef NO_OS
double* GxBus_ConfigGroupGetDouble(handle_t group,
		const char* key,
		double*     buf,
		double      dvalue)
{
	char str[INI_VALUE_LEN];
	char def[INI_VALUE_LEN];

	assert(buf != NULL);
	assert(key != NULL);

	snprintf(def, INI_VALUE_LEN, "%f", dvalue);

	if (GxBus_ConfigGroupGet(group, key, str, INI_VALUE_LEN, def) != NULL) {
		*buf = atof(str);
		return buf;
	}

	return NULL;
}
#endif

int32_t GxBus_ConfigGroupSet(handle_t group, const char* key, const char* value)
{
	// struct config_info*     config;
	struct config_group*    gp = (struct config_group*)group;
	char                    section[INI_SECT_LEN];
	char                    param[INI_PARM_LEN];

	assert(key != NULL);
	assert(value != NULL);

	if (key == NULL || value == NULL || group == GXCONFIG_FAILURE || gp == NULL) {
		return EINPUT;
	}

	gxbus_config_open();

	if (get_section(key, section, INI_SECT_LEN) == NULL
			|| get_param(key, param, INI_PARM_LEN) == NULL) {
		return EINPUT;
	}

	GXBUS_CONFIG_LOCK(gp);
	if (ini_set(gp->virtual_ini, section, param, value) < 0) {
		ini_append(gp->virtual_ini, section, param, value);
	}
	gp->changed_flag = TRUE;
	GXBUS_CONFIG_UNLOCK(gp);

	gxbus_config_close();
	return GXCONFIG_OK;
}

int32_t GxBus_ConfigGroupSetInt(handle_t group, const char* key, int32_t value)
{
	char str[INI_VALUE_LEN];

	snprintf(str, INI_VALUE_LEN, "%d", value);

	return GxBus_ConfigGroupSet(group, key, str);
}

int32_t GxBus_ConfigGroupSetDouble(handle_t group, const char* key, double value)
{
	char str[INI_VALUE_LEN];

	snprintf(str, INI_VALUE_LEN, "%f", value);

	return GxBus_ConfigGroupSet(group, key, str);
}

bool GxBus_ConfigGroupIsChanged(handle_t group)
{
	bool flag;
	struct config_group* gp = (struct config_group*)group;

	if (group == GXCONFIG_FAILURE || gp == NULL) {
		return FALSE;
	}

	GXBUS_CONFIG_LOCK(gp);
	flag = gp->changed_flag;
	GXBUS_CONFIG_UNLOCK(gp);

	return flag;
}

