/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"
#include "netapps/app_netapps_service.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <gx_apps.h>
#include "app_tools.h"
#include "common/gxpm.h"
#include <sys/select.h>
#include "devapi/gxirr_api.h"

#include "app_skin_setting.h"
#include "app_skin_xml.h"
#include "app_skin_config.h"
#include "app_skin_flash.h"
#include "app_skin_secure.h"
#include "app_skin_str_id.h"

#define SKIN_FLASH_SKIN_IMG_ENC_SUPPROT

#define SKIN_UPDATE_MODE_KEY                "skin>upmode"
#define SKIN_UPDATE_IMG_SEL_KEY             "skin>upsel"
#define SKIN_UPDATE_ERROR_FLAG_KEY          "skin>error_flag"
#define SKIN_NUMS_ERROR_FLAG_KEY            "skin>error_nums"
#define SKIN_FLASH_KEY_FIRST                "skin>flash_first"
#define SKIN_FLASH_KEY_ACTIVE_PARITION_ID   "skin>flash_actived"
#define SKIN_FLASH_KEY_INSTALLED_STATUS     "skin>flash_installed"

#define CHECK_PARTITION_ID(id) do\
{\
	if ( (id < 0) || (id>= s_skin_flash_info.partition_num ))\
	{\
		SKIN_ERR("[sk flash] check id err = %d,%s \n",id,__func__);\
		id = 0 ;\
	}\
} while(0)

typedef struct {
	unsigned int  partition_num ;
	unsigned int  active_partition_id  ;
	unsigned int  status[ALL_SKIN_PARTITION_MAX_NUM] ;
}skin_flash_data_info_t;

skin_flash_data_info_t s_skin_flash_info ;

//////////////////////////////////////////////////////////////////////////////

int  gxapp_skin_flash_set_update_mode(int mode)
{
	GxBus_ConfigSetInt(SKIN_UPDATE_MODE_KEY, mode);
	return 0 ;
}

int  gxapp_skin_flash_get_update_mode(void)
{
	int value_sel ;
	GxBus_ConfigGetInt(SKIN_UPDATE_MODE_KEY, &value_sel, 2);
	//SKIN_DBG("[sk flash] get update mode = %d \n",value_sel);
	return value_sel ;
}

int  gxapp_skin_flash_set_update_imgsel(int index)
{
	GxBus_ConfigSetInt(SKIN_UPDATE_IMG_SEL_KEY, index);
	return 0 ;
}

int  gxapp_skin_flash_get_update_imgsel(void)
{
	int value_sel = -1 ;
	GxBus_ConfigGetInt(SKIN_UPDATE_IMG_SEL_KEY, &value_sel, 0);
	//SKIN_DBG("[sk flash] get update img sel = %d \n",value_sel);
	return value_sel ;
}

int  gxapp_skin_flash_set_update_error_flag(int theme_id)
{
	GxBus_ConfigSetInt(SKIN_UPDATE_ERROR_FLAG_KEY, (theme_id+1));
	return 0;
}

int  gxapp_skin_flash_get_update_error_flag(void)
{
	int value_sel ;
	GxBus_ConfigGetInt(SKIN_UPDATE_ERROR_FLAG_KEY, &value_sel, 0);
	if ( value_sel > 0 )
	{
		SKIN_DBG("[skin flash] get update flag = %d \n",value_sel);
	}
	return value_sel ;
}

int  gxapp_skin_flash_clear_update_error_flag(void)
{
	GxBus_ConfigSetInt(SKIN_UPDATE_ERROR_FLAG_KEY, 0);
	return 0;
}

int  gxapp_skin_flash_recovery_info(void)
{
	GxBus_ConfigSetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, 0);
	GxBus_ConfigSetInt(SKIN_FLASH_KEY_INSTALLED_STATUS, FLASH_THEME_INSTALLED_STATUS);

	return 0 ;
}

static int _gxapp_skin_file_read(char *file_name,char *buffer,unsigned int size,char *src_md5)
{
	int  ret=0;
	FILE *fp = NULL;
	int len = 0;
	int nread = 0;

	fp = fopen(file_name,"rb");
	if(fp == NULL )
	{
		SKIN_ERR("[skin flash] #### file read err \n");
		return GXSKIN_IMAGE_FILE_OPEN_ERROR ;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if( (len<=0) || (len > size) )
	{
		fclose(fp);
		SKIN_ERR("[skin flash] #### file read , para = %d,%d \n",len,size);
		return GXSKIN_IMAGE_FILE_LEN_ERROR ;
	}

	SKIN_DBG("[skin flash] read img file,size=%d ... \n",len);
	nread = fread(buffer, 1, size, fp);
	if (nread <=0 )
	{
		fclose(fp);
		SKIN_ERR("[skin flash] #### file read , para = %d,%d \n",len,size);
		return GXSKIN_IMAGE_FILE_READ_ERROR ;
	}
	fclose(fp);
	
	ret = gxapp_skin_secure_buff_md5_check(buffer,nread,src_md5);
	if (ret < 0)//0 success
	{
		SKIN_ERR("[skin flash] #### buff md5 check err,nread=%d \n",nread);
		return GXSKIN_IMAGE_FILE_CHECK_MD5_ERROR ;
	}

	return nread ;
}


static int _gxapp_skin_flash_write_partition(int theme_id,char *buffer,unsigned int size,char *src_md5)
{
	int ret ;

	struct partition* flash_part = NULL;
    struct partition_info* partition = NULL;
	
	char parition_name[10] = {0} ;
	
	unsigned int flash_block_size = 0 ;
	int  start_addr_alignment = 1 ;
	enum GXPARTITION_FLASH_TYPE flash_type = 0 ;

	flash_part = GxCore_PartitionFlashInit();
	if (flash_part == NULL)
	{
		SKIN_ERR("[skin flash] #### flash init err \n");
		return GXSKIN_FLASH_PARTION_INIT_ERROR ;
	}
	sprintf(parition_name,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,theme_id);
	partition = GxCore_PartitionGetByName(flash_part, parition_name);
    if (partition == NULL)
	{
		SKIN_ERR("[skin flash] #### flash get name err \n");
		return GXSKIN_FLASH_PARTION_GET_NAME_ERROR ;
	}
	
	ret = GxCore_PartitionGetFlashBlockSize(&flash_block_size);
	if ( (ret != 0) || (flash_block_size <=0 ) )
	{
		SKIN_ERR("[skin flash] #### GetFlashBlockSize err %d \n",flash_block_size);
		return GXSKIN_FLASH_READ_ERROR;
	}
	ret = GxCore_PartitionGetFlashType(&flash_type);
	if (ret != 0)
	{
		SKIN_ERR("[skin flash] #### GetFlashType err \n");
		return GXSKIN_FLASH_READ_ERROR;
	}
	SKIN_DBG("[skin flash] base type=%d,block size=%d \n",flash_type,flash_block_size);
	if ( 0 != ((partition->start_addr) % flash_block_size) )
	{
		if ( flash_type == GXPARTITION_NANDFLASH )
		{
			SKIN_ERR("[skin flash] #### nand align err  \n");
			return GXSKIN_FLASH_ADDR_ALIGNMENT_ERROR;
		}
	    else
		{
			start_addr_alignment = 0 ;
		}
	}

	GxCore_Partition_Unprotect();
	
	SKIN_DBG("[skin flash] ready write, align=%d \n",start_addr_alignment);

	if ( start_addr_alignment == 1 )
	{
		SKIN_DBG("[skin flash] erase ... \n");
		ret = GxCore_PartitionErase(partition);
		if (ret == 1)
		{
			SKIN_DBG("[skin flash] writeraw ... \n");
			ret = GxCore_PartitionWriteRaw(partition, (void *)buffer, size, 0);
		}
	}
	else
	{
		SKIN_DBG("[skin flash] write ... \n");
		ret = GxCore_PartitionWrite(partition, (void *)buffer, size, 0);
	}
	if (ret == 0)//0 error //ret = 1 ; success
	{
        SKIN_ERR("[skin flash] #### flash wirte err \n");
        return GXSKIN_FLASH_WIRTE_ERROR;
    }

	if ( size >= 2 )
	{
		buffer[0] = 0x55 ;
		buffer[1] = 0xbb ;
	}
	//just for modify buffer. read buffer form flash

	SKIN_DBG("[skin flash] reading ... \n");

	ret = GxCore_PartitionRead(partition,(void *)buffer, size, 0);
	if (ret == 0)//0 error
	{
		SKIN_ERR("[skin flash] #### flash read err \n");
        return GXSKIN_FLASH_READ_ERROR;
	}

	ret = gxapp_skin_secure_buff_md5_check(buffer,size,src_md5);
	if (ret != 0)//0 success
	{
		SKIN_ERR("[skin flash] #### buff_md5_check err \n");
		return ret ;
	}

	return 0 ;
}

int  gxapp_skin_flash_update_partition(int theme_id,char *file_name,char *imd5,char *dmd5)
{
	GxHwMallocObj  hw_skin_e = {0};
#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
	GxHwMallocObj  hw_skin_d = {0};
#endif

	struct partition* flash_part = NULL;
    struct partition_info* partition = NULL;
	
	char parition_name[10] = {0} ;

	int  ret=0;
	int nread = 0 ;
	int active_id = 0 ;
	int error_code = -1 ;
	
	flash_part = GxCore_PartitionFlashInit();
	if (flash_part == NULL)
	{
		error_code = GXSKIN_FLASH_PARTION_INIT_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
	sprintf(parition_name,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,theme_id);
	partition = GxCore_PartitionGetByName(flash_part, parition_name);
    if (partition == NULL)
	{
		error_code = GXSKIN_FLASH_PARTION_GET_NAME_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}

	if ( imd5 == NULL ) 
	{
		error_code = GXSKIN_IMAGE_SRC_FILE_MD5_PARA_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
    if ( dmd5 == NULL ) 
	{
		error_code = GXSKIN_IMAGE_DRC_FILE_MD5_PARA_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
#endif

	hw_skin_e.size = partition->total_size+20;
    GxCore_HwMalloc(&hw_skin_e,MALLOC_WITH_CACHE);
    if(hw_skin_e.usr_p == NULL)
	{
		error_code = GXSKIN_IMAGE_SRC_FILE_MALLOC_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}

#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
	hw_skin_d.size = partition->total_size+20;
    GxCore_HwMalloc(&hw_skin_d,MALLOC_WITH_CACHE);
    if(hw_skin_d.usr_p == NULL)
	{
		GxCore_HwFree(&hw_skin_e);

		error_code = GXSKIN_IMAGE_DRC_FILE_MALLOC_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
#endif

	SKIN_DBG("[skin flash] check img md5 ... \n");

	nread = _gxapp_skin_file_read(file_name,hw_skin_e.usr_p,hw_skin_d.size,imd5);
	if ( nread <= 0 )
	{
		GxCore_HwFree(&hw_skin_e);
#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
		GxCore_HwFree(&hw_skin_d);
#endif
		error_code = nread ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}

#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
	SKIN_DBG("[skin flash] decrypt ... \n");
	gxapp_skin_secure_decrypt(hw_skin_e.usr_p,nread,hw_skin_d.usr_p);
	ret = gxapp_skin_secure_buff_md5_check(hw_skin_d.usr_p,nread,dmd5);
	if ( ret != 0 )
	{
		SKIN_ERR("[skin flash] #### update err -11, dst md5 sign err \n");
		GxCore_HwFree(&hw_skin_e);
		GxCore_HwFree(&hw_skin_d);

		error_code = GXSKIN_IMAGE_DESC_FILE_MD5_CHECK_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
#endif

	SKIN_DBG("[skin flash] update starting ... \n");

	active_id = gxapp_skin_flash_get_active_partition_id() ;

	gxapp_skin_flash_set_update_error_flag(theme_id);
	gxapp_skin_flash_set_status(theme_id,0);
	if ( theme_id == active_id)
	{
		gxapp_skin_flash_set_active_partition_id(theme_id);
	}

#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
	ret = _gxapp_skin_flash_write_partition(theme_id,hw_skin_d.usr_p,nread,dmd5);
#else
	ret = _gxapp_skin_flash_write_partition(theme_id,hw_skin_e.usr_p,nread,smd5);
#endif
	if ( ret != 0 )
	{
		SKIN_ERR("[skin flash] #### update flash_write_partition err \n");
		GxCore_HwFree(&hw_skin_e);
#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
		GxCore_HwFree(&hw_skin_d);
#endif
		return ret;
	}
	
	SKIN_DBG("[skin flash] finish ... \n");

	gxapp_skin_flash_set_status(theme_id,1);
	gxapp_skin_flash_clear_update_error_flag();

	GxCore_HwFree(&hw_skin_e);
#ifdef SKIN_FLASH_SKIN_IMG_ENC_SUPPROT
	GxCore_HwFree(&hw_skin_d);
#endif

	SKIN_DBG("[skin flash] update success, file name=%s,size=%d \n",file_name,nread);

	return 0 ;
}

#ifdef LINUX_OS
void _gxapp_skin_update_bin_proc(void* userdata)
{
	//SKIN_DBG("#### update bin proc \n");
}

void _gxapp_skin_update_bin_ok(void* userdata)
{
	// linux cmd_buf will reboot.
	SKIN_DBG("[skin flash] update bin ok \n");
	GxCore_ThreadDelay(2000);
	GxCore_Reboot();
}

#endif

int  gxapp_skin_flash_update_all(char *file_name)
{
#ifdef LINUX_OS
	char cmd_buf[256] = {0};

	gxapp_skin_app_close_all_before_reboot();

	sprintf(cmd_buf, "usb_update all \"%s\"", file_name);

	SKIN_DBG("[skin flash] linux cmd = %s \n",cmd_buf);

	system_shell(cmd_buf, 600000, _gxapp_skin_update_bin_proc, _gxapp_skin_update_bin_ok, NULL);
	
	return 0 ;
#endif

#ifdef ECOS_OS
	struct partition* flash_part = NULL;
    struct partition_info* partition = NULL;

	int  ret=0;	
	int error_code = -1 ;

	FILE *fp = NULL;
	int len = 0;
	unsigned int flash_size = 0;
	unsigned int write_size = 0;
	unsigned int left_block = 0;
	unsigned int block_size = 0;
	
	unsigned int total_read  = 0;
	unsigned int total_write = 0;
	int nread = 0 ;
	int i ;
	
	char *buffer = NULL ;
	char *write_buff = NULL ;
	int sys_block_size = 0 ;
	enum GXPARTITION_FLASH_TYPE flash_type = 0 ;

	if ( NULL == file_name )
		return GXSKIN_IMAGE_SRC_FILE_MD5_PARA_ERROR ;

	flash_part = GxCore_PartitionFlashInit();
	if (flash_part == NULL)
	{
		error_code = GXSKIN_FLASH_PARTION_INIT_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
	
	ret = GxCore_PartitionGetFlashType(&flash_type);
	if (ret != 0)
	{
		error_code = GXSKIN_FLASH_READ_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
	if ( flash_type == GXPARTITION_NORFLASH )
	{
		sys_block_size = 64*1024 ;
	}
	else
	{
		sys_block_size = 128*1024 ;
	}
	
	SKIN_ERR("[skin flash] type=%d, size=%d \n",flash_type,sys_block_size);
	
	for(i = 0; i < flash_part->count; i++)
	{
		partition = GxCore_PartitionGetByName(flash_part, flash_part->tables[i].name);
		if (partition == NULL)
		{
			error_code = GXSKIN_FLASH_PARTION_GET_NAME_ERROR ;
			SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
			return error_code;
		}

		flash_size += partition->total_size;
	}

	SKIN_ERR("[skin flash] file path = %s \n",file_name);

	fp = fopen(file_name,"rb");
	if(fp == NULL )
	{
		SKIN_ERR("[skin flash] #### file read err \n");
		return GXSKIN_IMAGE_FILE_OPEN_ERROR ;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if( (len<=0) || (len != flash_size) )
	{
		fclose(fp);
		SKIN_ERR("[skin flash] #### file read , para = %d,%d \n",len,flash_size);
		return GXSKIN_IMAGE_FILE_LEN_ERROR ;
	}
	
	SKIN_DBG("[skin flash] flash_size=%d,%dM \n",flash_size,flash_size/1024/1024);

	buffer = GxCore_Malloc(sys_block_size);
    if(buffer == NULL)
    {
        return GXSKIN_IMAGE_SRC_FILE_MALLOC_ERROR;
    }

	gxapp_skin_app_close_all_before_reboot();

	GxCore_Partition_Unprotect();

	SKIN_DBG("[skin flash] ready write ... \n");

	left_block = 0 ;

	for(i = 0; i < flash_part->count; i++)
	{
		partition = GxCore_PartitionGetByName(flash_part, flash_part->tables[i].name);
		if (partition == NULL)
		{
			//error_code = GXSKIN_FLASH_PARTION_GET_NAME_ERROR ;
			SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
			break;
		}

		flash_size = partition->total_size;
		write_size = 0 ;

		SKIN_ERR("[skin flash] table name=%s,size=0x%x,left=%d \n",flash_part->tables[i].name,flash_size,left_block);

		SKIN_DBG("[skin flash] erase ... \n");
		ret = GxCore_PartitionErase(partition);
		if (ret != 1)
		{
			//error_code = GXSKIN_FLASH_ERASE_ERROR ;
			SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
			break;
		}

		while(write_size<flash_size)
		{
			if (left_block <= 0 )
			{
				nread = fread(buffer, 1, sys_block_size, fp);
				if (nread <=0 )
				{
					SKIN_ERR("[skin flash] #### file read , para = %d,%d \n",write_size,flash_size);
					break;
				}
				write_buff = buffer ;
				total_read += sys_block_size ;
				//SKIN_ERR("[sk flash] flash read = %d \n",total_read);
			}
			else
			{
				write_buff = buffer + (sys_block_size - left_block) ;
				//SKIN_ERR("[sk flash] flash not read = %d \n",64*1024 - left_block);
			}

			if ( (flash_size - write_size) >= sys_block_size )
			{
				block_size = sys_block_size ;
			}
			else
			{
				block_size = flash_size - write_size ;
			}
			
			//SKIN_ERR("[sk flash] flash block_size AA = %d \n",block_size);

			if ( left_block <= 0 )
			{
				left_block = sys_block_size - block_size ; 
			}
			else
			{
				if ( block_size >= left_block )
				{
					block_size = left_block ;
					left_block = 0 ;
				}
				else
				{
					if ( sys_block_size > (left_block + block_size ) )
					{
						SKIN_ERR("[skin flash] file left_block = %d,%d \n",left_block,block_size);
						break;
					}
					left_block = sys_block_size - left_block - block_size ;
				}
			}

			//SKIN_ERR("[sk flash] flash block_size BB = %d \n",block_size);
			//SKIN_ERR("[sk flash] flash value = 0x%x,0x%x,0x%x,0x%x \n",write_buff[0],
						//write_buff[1],write_buff[2],write_buff[3]);

			if ( 0 == (write_size % (256*1024)) ) 
				SKIN_DBG("[skin flash] writeraw offset=%d \n",write_size);
			else if ((write_size+block_size) == flash_size)
				SKIN_DBG("[skin flash] writeraw block=%d,offset=%d \n",block_size,write_size);
			//ret = 1 ;
			ret = GxCore_PartitionWriteRaw(partition, (void *)write_buff, block_size, write_size);
			if (ret == 0)//0 error //ret = 1 ; success
			{
				SKIN_ERR("[skin flash] #### flash wirte err \n");
				break;
			}
			write_size+= block_size ;
			total_write += block_size ;

			//SKIN_ERR("[sk flash] flash wirte, block=0x%x, total=0x%x,left=0x%x \n",block_size,write_size,left_block);
		}
	}

	GxCore_Free(buffer);
	fclose(fp);

	SKIN_DBG("[skin flash] update success, size read=%d,write=%d \n",total_read,total_write);

	return 1 ;
#endif
}

int  gxapp_skin_flash_update_all_ext(char *bin,unsigned int size)
{
#ifdef LINUX_OS

	char cmd_buf[256] = {0};

	gxapp_skin_app_close_all_before_reboot();

	sprintf(cmd_buf, "net_update all \"%s\"", bin);

	SKIN_DBG("[skin flash] linux cmd = %s \n",cmd_buf);

	system_shell(cmd_buf, 600000, _gxapp_skin_update_bin_proc, _gxapp_skin_update_bin_ok, NULL);

	return 0 ;
#endif
#ifdef ECOS_OS

	struct partition* flash_part = NULL;
    struct partition_info* partition = NULL;

	int  ret=0;	
	int error_code = -1 ;

	unsigned int flash_size = 0;
	unsigned int write_size = 0;
	unsigned int left_block = 0;
	unsigned int block_size = 0;
	
	unsigned int total_read  = 0;
	unsigned int total_write = 0;
	int nread = 0 ;
	int i ;

	char *buffer = NULL ;
	char *write_buff = NULL ;
	int sys_block_size = 0 ;
	enum GXPARTITION_FLASH_TYPE flash_type = 0 ;

	if ( (NULL == bin) || (size <=0) )
		return GXSKIN_IMAGE_SRC_FILE_MD5_PARA_ERROR ;

	flash_part = GxCore_PartitionFlashInit();
	if (flash_part == NULL)
	{
		error_code = GXSKIN_FLASH_PARTION_INIT_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}

	ret = GxCore_PartitionGetFlashType(&flash_type);
	if (ret != 0)
	{
		error_code = GXSKIN_FLASH_READ_ERROR ;
		SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
		return error_code;
	}
	if ( flash_type == GXPARTITION_NORFLASH )
	{
		sys_block_size = 64*1024 ;
	}
	else
	{
		sys_block_size = 128*1024 ;
	}
	
	SKIN_ERR("[skin flash] type=%d, size=%d \n",flash_type,sys_block_size);

	SKIN_DBG("[skin flash] file size=%d,%dM \n",size,size/1024/1024);

	gxapp_skin_app_close_all_before_reboot();

	GxCore_Partition_Unprotect();
	
	SKIN_DBG("[skin flash] ready write ... \n");

	left_block = 0 ;
	nread = 0 ;

	buffer = bin ;

	for(i = 0; i < flash_part->count; i++)
	{
		partition = GxCore_PartitionGetByName(flash_part, flash_part->tables[i].name);
		if (partition == NULL)
		{
			//error_code = GXSKIN_FLASH_PARTION_GET_NAME_ERROR ;
			SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
			break;
		}

		flash_size = partition->total_size;
		write_size = 0 ;

		SKIN_ERR("[skin flash] table name=%s,size=0x%x,left=%d \n",flash_part->tables[i].name,flash_size,left_block);

		SKIN_DBG("[skin flash] erase ... \n");
		ret = GxCore_PartitionErase(partition);
		if (ret != 1)
		{
			//error_code = GXSKIN_FLASH_ERASE_ERROR ;
			SKIN_ERR("[skin flash] #### update err code = %d \n",error_code);
			break;
		}

		while(write_size<flash_size)
		{
			if (left_block <= 0 )
			{
				if (nread >= size )
				{
					SKIN_ERR("[skin flash] #### file read , para = %d,%d \n",write_size,flash_size);
					break;
				}
				buffer = bin + nread ;
				nread += sys_block_size ;

				write_buff = buffer ;
				total_read += sys_block_size ;
				//SKIN_ERR("[sk flash] flash read = %d \n",total_read);
			}
			else
			{
				write_buff = buffer + (sys_block_size - left_block) ;
				//SKIN_ERR("[sk flash] flash not read = %d \n",64*1024 - left_block);
			}

			if ( (flash_size - write_size) >= sys_block_size )
			{
				block_size = sys_block_size ;
			}
			else
			{
				block_size = flash_size - write_size ;
			}
			
			//SKIN_ERR("[sk flash] flash block_size AA = %d \n",block_size);

			if ( left_block <= 0 )
			{
				left_block = sys_block_size - block_size ; 
			}
			else
			{
				if ( block_size >= left_block )
				{
					block_size = left_block ;
					left_block = 0 ;
				}
				else
				{
					if ( sys_block_size > (left_block + block_size ) )
					{
						SKIN_ERR("[skin flash] file left_block = %d,%d \n",left_block,block_size);
						break;
					}
					left_block = sys_block_size - left_block - block_size ;
				}
			}

			//SKIN_ERR("[sk flash] flash block_size BB = %d \n",block_size);
			//SKIN_ERR("[sk flash] flash value = 0x%x,0x%x,0x%x,0x%x \n",write_buff[0],
						//write_buff[1],write_buff[2],write_buff[3]);

			if ( 0 == (write_size % (256*1024)) ) 
				SKIN_DBG("[skin flash] writeraw offset=%d \n",write_size);
			else if ((write_size+block_size) == flash_size)
				SKIN_DBG("[skin flash] writeraw block=%d,offset=%d \n",block_size,write_size);
			//ret = 1 ;
			ret = GxCore_PartitionWriteRaw(partition, (void *)write_buff, block_size, write_size);
			if (ret == 0)//0 error //ret = 1 ; success
			{
				SKIN_ERR("[skin flash] #### flash wirte err \n");
				break;
			}
			write_size+= block_size ;
			total_write += block_size ;

			//SKIN_ERR("[sk flash] flash wirte, block=0x%x, total=0x%x,left=0x%x \n",block_size,write_size,left_block);
		}
	}

	SKIN_DBG("[skin flash] update success, size read=%d,write=%d \n",total_read,total_write);

	return 1 ;
#endif
}

//////////////////////////////
//////////////////////////////

static void _app_skin_get_skin_flash_partition_num(void)
{
	int num = 0 ;
#ifdef SUPPORT_USER_SKIN_PARTITION_NUM_FROM_DEFINE
    if ( USER_SKIN_PARTITION_MAX_NUM > ALL_SKIN_PARTITION_MAX_NUM )
		num = ALL_SKIN_PARTITION_MAX_NUM ;
	else
		num = USER_SKIN_PARTITION_MAX_NUM ;
#else
	int i = 0;
	char name[20] ;
	struct partition* flash_table = NULL;
    flash_table = GxCore_PartitionFlashInit();
	if (flash_table != NULL)
	{
		memset(name,0,20);
		sprintf(name,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,num);
		SKIN_DBG("[skin flash] table count=%d, first=%s \n",flash_table->count,name);
		for (i = 0; i < flash_table->count; i++)
		{
			if (strncasecmp(flash_table->tables[i].name, name, 4) == 0)
			{
				num++ ;
				memset(name,0,20);
				sprintf(name,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,num);

				SKIN_DBG("[skin flash] id=%d,num=%d,next=%s \n",i,num,name);
			}
			else
			{
				if ( num > 0 )
				{
					SKIN_DBG("[skin flash] break find num, id=%d \n",i);
					break;
				}
			}
		}
	}
	else
	{
		SKIN_ERR("[skin flash] flash partition num err \n");
	}
#endif
	if ( num <=0 )
	{
		SKIN_ERR("[skin flash] #### init parition num err \n");
		gxapp_skin_flash_set_update_error_flag(0);
	}
	else if ( num > ALL_SKIN_PARTITION_MAX_NUM )
	{
		SKIN_ERR("[skin flash] init parition num warning err2 \n");
		num = ALL_SKIN_PARTITION_MAX_NUM;
	}
	s_skin_flash_info.partition_num = num ;

	//SKIN_DBG("[sk flash] get parition num = %d \n",num);
}

static void _app_skin_flash_flash_info_init(void)
{
	int i ;

	for(i=0;i<ALL_SKIN_PARTITION_MAX_NUM;i++)
	{
		s_skin_flash_info.status[i] = 0 ;
	}

	s_skin_flash_info.status[0] = 1 ;
	s_skin_flash_info.active_partition_id = 0 ;
	s_skin_flash_info.partition_num = 1 ;

	_app_skin_get_skin_flash_partition_num();
}

static int _app_skin_flash_partition_init(void)
{
	int ret ;
    int num = 0 ;
	int value = 0  ;
	int i ;
	int active_id = 0 ;
	int err_id = 0 ;
	char theme_part[10] = {0} ;

	_app_skin_flash_flash_info_init();
	
	num = s_skin_flash_info.partition_num ;

	SKIN_DBG("[skin flash] init parition = %d \n",num);
	
	if (num <= 0 )
		return 0 ;

	GxBus_ConfigGetInt(SKIN_FLASH_KEY_FIRST, &value, 0);
	if ( value == 0 ) //first
	{
		SKIN_DBG("[skin flash] #### first \n");
		GxBus_ConfigSetInt(SKIN_FLASH_KEY_FIRST, 1);

		GxBus_ConfigGetInt(SKIN_FLASH_KEY_INSTALLED_STATUS, &value, FLASH_THEME_INSTALLED_STATUS);

		SKIN_DBG("[skin flash] first active_id=%d , status=0x%x \n",active_id,value);
		if ((value&0x1) == 1)
		{
			for(i=1;i<num;i++)
			{
				s_skin_flash_info.status[i] = ((value>>i)&1) ;
			}
		}
	}
	else
	{
		//SKIN_DBG("[sk flash] second \n");

		GxBus_ConfigGetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, &active_id, 0);
		GxBus_ConfigGetInt(SKIN_FLASH_KEY_INSTALLED_STATUS, &value, FLASH_THEME_INSTALLED_STATUS);

		SKIN_DBG("[skin flash] init active_id=%d , status=0x%x \n",active_id,value);

		for(i= 0;i<num;i++)
		{
			s_skin_flash_info.status[i] = ((value>>i)&1) ;
		}

		if ( (active_id < 0) || (active_id>= num ))
		{
			SKIN_ERR("[skin flash] #### error1,need to burn = %d \n",active_id);
			gxapp_skin_flash_set_update_error_flag(0);
			s_skin_flash_info.active_partition_id = 0 ;
			GxBus_ConfigSetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, 0);
			return -1 ;
		}

		err_id = gxapp_skin_flash_get_update_error_flag() ;
		SKIN_ERR("[skin flash] init, update flag=%d \n",err_id);
		if ( (err_id < 0) || (err_id > num) )
		{
			SKIN_ERR("[skin flash] #### error2,need to burn \n");
			gxapp_skin_flash_set_update_error_flag(0);
			s_skin_flash_info.active_partition_id = 0 ;
			GxBus_ConfigSetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, 0);
			return -1 ;
		}

		s_skin_flash_info.active_partition_id = active_id ;

		if ( err_id > 0 )
		{
			s_skin_flash_info.status[err_id-1] = 0 ;
			if ( (err_id-1) == active_id )
			{
				SKIN_DBG("[skin flash] init, active id is update err id \n");
				for(i= 0;i<num;i++)
				{
					if ( i == active_id )
						continue ;
				
					if ( s_skin_flash_info.status[i] )
					{
						SKIN_DBG("[skin flash] init, active id switch %d to %d \n",active_id,i);

						gxapp_skin_flash_set_active_partition_id(i);
						active_id = i ;
						s_skin_flash_info.active_partition_id = active_id ;						
						break;
					}
				}

				if (i >= num )
				{
					SKIN_ERR("[skin flash] #### init, active is null,ready to recovery mode\n");
					return -2;
				}
			}
			else
			{
				if ( num > 1)
				{
					SKIN_ERR("[skin flash] init clean update flag, err id=%d \n",err_id);
					gxapp_skin_flash_clear_update_error_flag();
				}
				else
				{
					SKIN_ERR("[skin flash] #### init unknow error,ready to recovery mode\n");
					return -3;
				}
			}
		}
	}

	sprintf(theme_part,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,s_skin_flash_info.active_partition_id);

	SKIN_DBG("[skin flash] init active id=%d, %s \n",s_skin_flash_info.active_partition_id,theme_part);
	for(i= 0;i<num;i++)
	{
		SKIN_DBG("[skin flash] i=%d,status=%d \n",i,s_skin_flash_info.status[i]);
	}

	ret = gxapp_skin_flash_mount(theme_part,ACTIVE_MOUNT_THEME_PATH);
	if (ret == 0 )
	{
		ret = access(ACTIVE_FLASH_THEME_BASE_PATH, F_OK) ;
		SKIN_DBG("[skin flash] init file access ret =%d, %s \n",ret,ACTIVE_FLASH_THEME_BASE_PATH);
		if (ret != 0 )
		{
			gxapp_skin_flash_unmount(ACTIVE_MOUNT_THEME_PATH);
		}
	}
	if (ret == 0)
	{
		gxapp_skin_flash_clear_update_error_flag();
		SKIN_DBG("[skin flash] init success\n");
		return 0 ;
	}

	for(i= 0;i<num;i++)
	{
		if ( i == active_id )
			continue ;

		if ( s_skin_flash_info.status[i] )
		{
			SKIN_DBG("[skin flash] init B, mount switch, para = %d,%d \n",active_id,i);

			ret = gxapp_skin_flash_mount(theme_part,ACTIVE_MOUNT_THEME_PATH);
			if (ret == 0 )
			{
				ret = access(ACTIVE_FLASH_THEME_BASE_PATH, F_OK) ;
				SKIN_DBG("[skin flash] init B, access ret =%d, %s \n",ret,ACTIVE_FLASH_THEME_BASE_PATH);
				if (ret != 0 )
				{
					gxapp_skin_flash_unmount(ACTIVE_MOUNT_THEME_PATH);
				}
			}
			if (ret == 0)
			{
				SKIN_DBG("[skin flash] init B, success\n");
				gxapp_skin_flash_set_active_partition_id(i);
				active_id = i ;
				s_skin_flash_info.active_partition_id = active_id ;
				if (err_id)
				{
					gxapp_skin_flash_clear_update_error_flag();
				}
				return 0 ;
			}

			s_skin_flash_info.status[i] = 0 ;
		}
	}

	return 1 ;
}

#ifdef SUPPORT_HELP_KEY
static int _app_skin_read_key(void)
{
	unsigned int code = 0;
    static int irr_fd = -1;
    static int panel_fd = -1;
    int max_fd = -1;
    fd_set fds;
    struct timeval timeout;
	struct timeval first_timeout;
	
	unsigned short syscode = 0;
    unsigned short syscode_reverse = 0;
    unsigned short usrcode = 0;
    unsigned short usrcode_reverse = 0;
	
	unsigned int key_value = 0 ;
	unsigned int i =0 ;
	unsigned int key_1 = 0 ;
	unsigned int key_2 = 0 ;

#if 1
	int sensitivity = 7;    //irr_simple_ignore_num is (MAX_SENS  - sensitivity) 3
    int ret;
	SKIN_ERR("[skin flash] key set sensitivity \n");
    ret=GxIRR_Init();
    if(ret == 0)
    {
        if(GxIRR_SetSensitivity(sensitivity) != 0)
        {    
			SKIN_ERR("[skin flash] >>>>>key config irr err<<<<<<\n");
		}
        GxIRR_Close();
    }
    else
	{
        SKIN_ERR("[skin flash] >>>>>key open irr err<<<<<<\n");
	}
#endif

	SKIN_ERR("[skin flash] key start \n");
    FD_ZERO(&fds);

    if (irr_fd == -1) {
        irr_fd = GxCore_Open("/dev/gxirr0", "r");
        if(irr_fd < 0)
        {
            SKIN_ERR("[skin flash] open irr err \n");
            irr_fd = -2;
        }
    }

    if (panel_fd == -1) {
        panel_fd = GxCore_Open("/dev/gxpanel0", "r");
        if(panel_fd < 0) {
            SKIN_ERR("[skin flash] open panle err \n");
            panel_fd = -2;
        }
    }

    if (irr_fd > 0) {
        FD_SET(irr_fd, &fds);
        if (irr_fd > max_fd)
            max_fd = irr_fd;
    }
	
	if (panel_fd > 0) {
        FD_SET(panel_fd, &fds);
        if (panel_fd > max_fd)
            max_fd = panel_fd;
    }

	SKIN_ERR("[skin flash] key ready,handle=%d,%d,%d \n",irr_fd,panel_fd,max_fd);

	timeout.tv_sec = 0;
    timeout.tv_usec = 60 * 1000;

	first_timeout.tv_sec = 0;
    first_timeout.tv_usec = 200 * 1000;

	for(i=0;i<HELP_KEY_READ_TIMES;i++)
	{
		code = 0 ;
		
		if ( i == 0 )
		{
			ret = select(max_fd + 1, &fds, NULL, NULL, &first_timeout) ;
		}
		else
		{
			ret = select(max_fd + 1, &fds, NULL, NULL, &timeout);
		}

		if ( ret > 0)
		{
			if (FD_ISSET(irr_fd, &fds)) {
				GxCore_Read(irr_fd, &code, sizeof(code), 1);
			} else if (FD_ISSET(panel_fd, &fds)) {
				GxCore_Read(panel_fd, &code, sizeof(code), 1);
			}

			if (code) 
			{
				SKIN_ERR("[skin flash] key code=0x%x,i=%d \n",code,i);
				//GxTime time;
				if(code > 0xFFFF)
				{
					syscode = (code >> 24) & 0xff;
					syscode_reverse = (code >> 16) & 0xff;
					usrcode = (code >> 8) & 0xff;
					usrcode_reverse = (code)  & 0xff;

					if(0xff != syscode + syscode_reverse)
					{
						SKIN_ERR("[skin flash] syscode verify error: 0x%x, 0x%x\n", syscode, syscode_reverse);
					}

					if(0xff != usrcode + usrcode_reverse)
					{
						SKIN_ERR("[skin flash] usrcode verify error: 0x%x, 0x%x\n", usrcode, usrcode_reverse);
						continue;
					}

					key_value = (syscode<<8) + usrcode;
				}
				else
				{
					key_value = code ;
				}

				if ( IS_FIRST_KEY(key_value) )
				{
					if (key_1 == 0)
					{
						key_1 = key_value ;
					}
				}
				else
				{
					if (key_1 != 0)
					{
						key_2 = key_value ;
					}
				}

				SKIN_ERR("[skin flash] key check = 0x%x,0x%x \n",key_1,key_2);
				if ( IS_HELP_KEY(key_1,key_2) )
				{
					SKIN_ERR("[skin flash] key into help \n");
					GxCore_Close(irr_fd);
					GxCore_Close(panel_fd);
					return -1 ;
				}
			}
		}

		if (key_1 == 0 )
		{
			SKIN_ERR("[skin flash] key exit,i=%d \n",i);
			break;
		}
	}

	if (i>=HELP_KEY_READ_TIMES)
	{
		SKIN_ERR("[skin flash] key time out,[0x%x,0x%x] \n",key_1,key_2);
	}

	GxCore_Close(irr_fd);
	GxCore_Close(panel_fd);

	return 0 ;
}
#endif

int  gxapp_skin_flash_partition_init(void)
{
	int value = 0 ;
	int ret  = 0  ;

#ifdef SUPPORT_HELP_KEY
	ret = _app_skin_read_key();
	if (ret < 0 )
	{
		SKIN_ERR("[skin flash] init, check help key, force torecovery mode \n");
		gxapp_skin_flash_set_update_error_flag(0);
		return ret ;
	}
#endif

	ret = _app_skin_flash_partition_init();

	if (ret > 0 )
	{
		GxBus_ConfigGetInt(SKIN_NUMS_ERROR_FLAG_KEY, &value, 1);
		SKIN_ERR("[skin flash] init, err times = %d , ret = %d \n",value,ret);
		if (value >= ACTIVE_FLASH_THEME_ERR_NUM_OVER_RECOVERY )
		{
			gxapp_skin_flash_set_update_error_flag(0);
		}
		else
		{
			value +=1 ;
			GxBus_ConfigSetInt(SKIN_NUMS_ERROR_FLAG_KEY, value);
		}
	}
	
	return ret ;
}

int  gxapp_skin_flash_other_mount(int parition_id)
{
	char path[FULL_XML_PATH_LENGTH];

	memset(path,0,FULL_XML_PATH_LENGTH);
	snprintf(path, FULL_XML_PATH_LENGTH, "%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,parition_id);

	gxapp_skin_flash_mount(path,OTHER_MOUNT_THEME_PATH);
	
	return 0 ;
}

int  gxapp_skin_flash_mount(char *parition_name,char *mount_path)
{
	int ret = 0 ;
	if ( (parition_name == NULL) || (mount_path == NULL))
		return -1 ;

	SKIN_DBG("[skin flash] mount start: %s to %s \n",parition_name,mount_path);

    struct partition* flash_part = NULL;
	flash_part = GxCore_PartitionFlashInit();
    if (!flash_part)
	{
		SKIN_ERR("[sk flash] GxCore_PartitionFlashInit error \n");
		return -2 ;
	}
	
#ifdef ECOS_OS
	struct partition_info* partition = NULL;
    char *fs_type = NULL;
	static char dir_cur[32] = {0};

	extern int mount(const char *devname,
            const char *dir,
            const char *fsname,
            unsigned long mountflags,
            const void *data);

	partition = GxCore_PartitionGetByName(flash_part, parition_name);
    if (partition)
    {
        if (partition->file_system_type < MINIFS)
        {
            fs_type = GxCore_PartitionGetType(partition);

            sprintf(dir_cur, "/dev/flash/0/0x%x,0x%x", partition->start_addr, partition->total_size);

            //gxapp_skin_flash_unmount(mount_path);
            ret = mount(dir_cur, mount_path, fs_type, 0, 0);

            SKIN_DBG("[skin flash] mount ret=%d, dir_cur=%s, type=%s\n",ret,dir_cur,fs_type);
				
			return ret ;
        }
		else
		{
			SKIN_ERR("[skin flash] partition->file_system_type = %d error \n",partition->file_system_type);
	    }
    }
	else
	{
		SKIN_ERR("[skin flash] GxCore_PartitionGetByName error \n");
	}
#else

	int i ;
	char cmd[100] = {0};

	for (i = 0; i < flash_part->count; i++)
	{
		if (strncasecmp(flash_part->tables[i].name, parition_name, 5) == 0)
		{
			sprintf(cmd, "mount /dev/mtdblock%d %s -t squashfs", i,mount_path);

			SKIN_DBG("[skin flash] mtd id=%d, cmd=%s \n",i,cmd);
				
			//gxapp_skin_flash_unmount(mount_path);
			ret = system(cmd);

			SKIN_DBG("[skin flash] mount ret=%d\n",ret);
				
			return ret ;
		}
	}
	SKIN_DBG("[skin flash] mount can not find partition \n");
#endif

	return -3 ;
}

int  gxapp_skin_flash_unmount(char *mount_path)
{
	int ret = 0 ;
	if ( mount_path == NULL )
		return -1 ;
	
#ifdef ECOS_OS
    extern int umount(const char *name);
	ret = umount(mount_path);
#else
	
    char cmd[100] = {0};

	sprintf(cmd, "umount %s", mount_path);

	SKIN_DBG("[skin flash] umount cmd = %s \n",cmd);
				
	ret = system(cmd);

#endif

	SKIN_DBG("[skin flash] umout ret=%d, path=%s \n",ret,mount_path);

	return ret ;
}

int  gxapp_skin_flash_get_status(int theme_id)
{
	CHECK_PARTITION_ID(theme_id);
	
	return s_skin_flash_info.status[theme_id] ;
}

int  gxapp_skin_flash_get_active_partition_id(void)
{
	int active_partition_id ;

	active_partition_id = s_skin_flash_info.active_partition_id ;
	
	CHECK_PARTITION_ID(active_partition_id);

	//SKIN_DBG("[sk flash] get active partition id = %d \n", active_partition_id);
	return  active_partition_id ;
}

int  gxapp_skin_flash_set_status(int theme_id,int installed)
{
	int value ;
	int new_value ;
	int bit_value ;

	CHECK_PARTITION_ID(theme_id);

	bit_value = installed & 1 ;

	GxBus_ConfigGetInt(SKIN_FLASH_KEY_INSTALLED_STATUS, &value, FLASH_THEME_INSTALLED_STATUS);

    if ( bit_value )
	{
		s_skin_flash_info.status[theme_id] = 1 ;
		new_value = value | (1<<theme_id );
	}
	else
	{
		s_skin_flash_info.status[theme_id] = 0 ;
		new_value = value & (~(1<<theme_id));
	}

	SKIN_DBG("[skin flash] set status id=%d,statue=%d, value=0x%x,0x%x \n",theme_id,installed,value,new_value);

	if ( value != new_value )
	{
		GxBus_ConfigSetInt(SKIN_FLASH_KEY_INSTALLED_STATUS, new_value);
	}

	return 0 ;
}

int  gxapp_skin_flash_set_active_partition_id(int theme_id)
{
	int old_id ;
	int new_id ;

	CHECK_PARTITION_ID(theme_id);

	//GxBus_ConfigGetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, &old_id, 0);
	old_id = s_skin_flash_info.active_partition_id ;
	new_id = theme_id ;
	
	if (old_id != new_id )
	{
		s_skin_flash_info.active_partition_id = new_id ;
		GxBus_ConfigSetInt(SKIN_FLASH_KEY_ACTIVE_PARITION_ID, new_id);
	}

	SKIN_DBG("[skin flash] set active partition %d, %d \n",old_id,new_id);

	return 0 ;
}

static void _gxapp_skin_flash_set_simple_name(int i, skin_item_node_t *item_data)
{
	char *p_name = NULL;
	p_name = GxCore_Calloc(sizeof(char), 100);
	if(NULL == p_name)
	{
		return  ;
	}
	snprintf(p_name, 100, "default%d",i);
	SKIN_DBG("[skin flash] set name, id=%d \n",i);

	if ( item_data->name == NULL )
	{
		item_data->name = GxCore_Strdup(p_name) ;
	}

	if (item_data->simg_url == NULL)
	{
		memset(p_name,0,100);
		snprintf(p_name, 100, "%s (active)", item_data->name);
		item_data->simg_url = GxCore_Strdup(p_name);
	}
	APP_FREE(p_name);
}

static void _gxapp_skin_flash_set_simple_preveiw(int active,skin_item_node_t *item_data)
{
	int ret  = -1;
	int i = 0 ;
	int j = 0 ;
	int num = 0 ;

	char *pic_type[3] ;

	int  length = 0 ;
	char *path = NULL ;
	char *file_name = NULL ;
	
	pic_type[0] = ".jpg" ;
	pic_type[1] = ".bmp" ;
	pic_type[2] = ".png" ;
	
	if ( active > 0)
	{
		length = strlen(ACTIVE_FLASH_PREVIEW_PATH)+15;
		path = ACTIVE_FLASH_PREVIEW_PATH ;
	}
	else
	{
		length = strlen(OTHER_FLASH_PREVIEW_PATH)+15;
		path = OTHER_FLASH_PREVIEW_PATH ;
	}

	file_name = GxCore_Calloc(sizeof(char), length);
	if(NULL == file_name)
	{
		SKIN_ERR("[skin flash] No memory to GxCore_Calloc = %d \n",length);
		return ;
	}

	for(i=0; i<FLASH_PREVIEW_NODE_NUM; i++)
	{
		for(j=0;j<3;j++)
		{
			memset(file_name,0,length);
			snprintf(file_name, length, "%s/preview%d%s", path,i,pic_type[j]);
			ret = access(file_name, F_OK) ;
			if ( ret == 0 )
			{
				num++;
				item_data->preview[i].pre_url = GxCore_Strdup(file_name);
				SKIN_DBG("[skin flash] set preview,active flag=%d,num=%d \n",active,num);
				SKIN_DBG("[skin flash] set preview,i=%d,path=%s \n",i,item_data->preview[i].pre_url);
				break ;
			}
			/*else
			{
				SKIN_DBG("[sk flash] set preview,i=%d,path=%s not exist \n",i,file_name);
			}*/
		}
	}

	APP_FREE(file_name);
}

static void _gxapp_skin_flash_set_simple_data(int active,int i, skin_item_node_t *item_data)
{
	// flash theme下可以没有skin.xml, ret & 0x1,name, ret&0x6 preveiw

	_gxapp_skin_flash_set_simple_name(i,item_data);

	_gxapp_skin_flash_set_simple_preveiw(active,item_data);

	gxapp_set_installed_uiid(i,item_data->uiid);

	SKIN_DBG("[skin flash] set data,i=%d, uiid = %d \n",i,item_data->uiid);
}

int  gxapp_skin_flash_get_data(skin_data_node_t *pdata)
{
	int i ;
	int active_id = 0 ;
	int num = 0 ;
	int ret = 0 ;
	int err = 0 ;
	char theme_part[20] ;

	num = s_skin_flash_info.partition_num ;
	active_id = s_skin_flash_info.active_partition_id ;
	
	SKIN_DBG("[skin flash] get data,num=%d, active partition id = %d \n",num,active_id);

	if (num <= 0 )
		return -1 ;

	pdata->node = (skin_item_node_t *)malloc(num * sizeof(skin_item_node_t));
	if ( pdata->node == NULL)
	{
		pdata->num = 0 ;
		return -2;
	}
	memset(pdata->node, 0, num * sizeof(skin_item_node_t));
	pdata->num = num ;

    for(i=0;i<num;i++)
    {
		if ( 0 == s_skin_flash_info.status[i] )
		{
			pdata->node[i].name = GxCore_Strdup("(no skin)");
			continue ;
		}
		if (i == s_skin_flash_info.active_partition_id )
		{
			ret = gxapp_file_get_simple_data(ACTIVE_FLASH_THEME_XML_PATH,i,&pdata->node[i]);
			_gxapp_skin_flash_set_simple_data(1,i,&pdata->node[i]);
			continue ;
		}
		SKIN_DBG("[skin flash] get data, flash parition id = %d \n",i);

		memset(theme_part,0,20);
		sprintf(theme_part,"%s%d",SKIN_BASE_FLASH_THEME_PARITION_NAME,i);

		ret = gxapp_skin_flash_mount(theme_part,OTHER_MOUNT_THEME_PATH);
		if (ret != 0 )
		{
			err |= 1 ;
			SKIN_ERR("[skin flash] #### get data, mount err, ret=%d, i=%d \n",ret,i);
			gxapp_skin_flash_set_status(i,0);
			pdata->node[i].name = GxCore_Strdup("(no skin)");
			continue;
		}
		ret = access(OTHER_FLASH_THEME_BASE_PATH, F_OK) ;
		if (ret != 0 )
		{
			err |= 2 ;
			SKIN_ERR("[skin flash] #### get data, file err,i=%d, %s \n",i,OTHER_FLASH_THEME_BASE_PATH);
			gxapp_skin_flash_set_status(i,0);
			pdata->node[i].name = GxCore_Strdup("(no skin)");
			gxapp_skin_flash_unmount(OTHER_MOUNT_THEME_PATH);
			continue;
		}

		ret = gxapp_file_get_simple_data(OTHER_FLASH_THEME_XML_PATH,i,&pdata->node[i]);
		_gxapp_skin_flash_set_simple_data(0,i,&pdata->node[i]);

		gxapp_skin_flash_unmount(OTHER_MOUNT_THEME_PATH);
    }

	return err ;
}

int  gxapp_skin_flash_get_recovry_status(void)
{
	int err_id = 0 ;

	err_id = gxapp_skin_flash_get_update_error_flag();

	if ( err_id > 0 )
	{
		SKIN_ERR("[skin flash] #### get recovery status = %d \n",err_id);

		SKIN_ERR("[skin flash] #### gui to recovery menu \n");

		GUI_CreateDialog(WND_SKRECOVERY);

		return 1 ;
	}

	GUI_LinkDialog(WND_SKIN, "skin/wnd_skin.xml");
	GUI_LinkDialog(WND_SKPREVIEW, "skin/wnd_skpreview.xml");

	return 0 ;
}

char* gxapp_skin_flash_get_err_tips(int ret)
{
	static char err_tips[50] ;
	memset(err_tips,0,50);
	
	if (ret == GXSKIN_IMAGE_SRC_FILE_MD5_PARA_ERROR )
	{
		return SKIN_STR_ID_ERR_ERR_PARA ;
	}
	else if (ret == GXSKIN_IMAGE_DRC_FILE_MD5_PARA_ERROR )
	{
		return SKIN_STR_ID_ERR_DMD5_PARA ;
	}
	else if (ret == GXSKIN_IMAGE_FILE_OPEN_ERROR )
	{
		return SKIN_STR_ID_ERR_FILE_OPEN ;
	}
	else if (ret == GXSKIN_IMAGE_FILE_LEN_ERROR )
	{
		return SKIN_STR_ID_ERR_FILE_SIZE ;
	}
	else if (ret == GXSKIN_IMAGE_FILE_READ_ERROR )
	{
		return SKIN_STR_ID_ERR_FILE_READ ;
	}
	else if (ret == GXSKIN_IMAGE_FILE_CHECK_MD5_ERROR )
	{
		return SKIN_STR_ID_ERR_SMD5_NOT_MATCH ;
	}
	else if (ret == GXSKIN_IMAGE_DESC_FILE_MD5_CHECK_ERROR )
	{
		return SKIN_STR_ID_ERR_DMD5_NOT_MATCH ;
	}
	else
	{
		snprintf(err_tips, 50, "Skin install failer: error code=%d",ret);
	}

	return err_tips;
}

//--------------------------------------------------------

#endif
