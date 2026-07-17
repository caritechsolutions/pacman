#ifdef NO_OS
#include "gxloader.h"
#endif
#include <string.h>
#include "common.h"
#include "config.h"
#include "gxdlp_api.h"
#include "ini.h"
#include "common/gx_crc32.h"
#include "upgrade.h"
#include "installer.h"
#include "gxupdate_api.h"
#include "progress.h"
#include "devapi/gxflash_api.h"

#define UPDATE_IMAGE_HEADER_LEN 4096
#define UPDATE_IMAGE_MAGIC_NUM1 0xEBD88320
#define UPDATE_IMAGE_MAGIC_NUM2 0x12477CDF
struct update_image_header {
	unsigned int magicnum1;
	unsigned int magicnum2;
	unsigned char version;
	unsigned int image_size;
	unsigned int image_crc32;
	unsigned char image_comp;
	unsigned int reserved1;
	unsigned int reserved2;
	unsigned int mid;
	unsigned int hwversion;
	unsigned int swversion;
	unsigned char ota_type;
	unsigned int sn_start;
	unsigned int sn_end;
	unsigned int oui;
	unsigned int operator_id;
	unsigned int crc32;
} __attribute__ ((packed));

#define MERGE_HEAD_MAGIC              "mergebin"
#define MERGE_HEAD_MAGIC_SIZE         8
#define FILE_NAME_MAX_LEN               256
struct merge_sub_file_s {
	char name[FILE_NAME_MAX_LEN];
	uint32_t start_addr;
	uint32_t len;
};

struct merge_head_s {
	char magic[MERGE_HEAD_MAGIC_SIZE];
	int sub_file_num;
};

static bool upgrade_all_bin = false;
static struct partition *flash_table = NULL;
static struct partition mem_table;

GxUpdateImageInfo image_info;
static GxUpdateOTAImg ota_img;

int GxUpdate_GetImageInfo(GxUpdateImageInfo *info)
{
	if (info == NULL) {
		gxloge("param info is NULL\n");
		return -2;
	}

	memcpy(info, &image_info, sizeof(image_info));

	return 0;
}

int GxUpdate_GetOTAImg(GxUpdateOTAImg *img)
{
	GxUpdateMsg msg;
	if (img == NULL) {
		gxloge("param img is NULL\n");
		return -2;
	}

	update_get_cur_progress(&msg);
	if (msg.status != GXUPDATE_PARSE_OK) {
		gxloge("GxUpdate_GetOTAImg work well only at GXUPDATE_PARSE_OK\n");
		return -1;
	}
	if (ota_img.image_data == NULL) {
		gxloge("update image not contain OTA img\n");
		return -1;
	}
	memcpy(img, &ota_img, sizeof(ota_img));

	return 0;
}

bool image_version_is_ok(GxUpdateImageInfo *image, int print_version)
{
	uint32_t hwversion = 0,swversion = 0, manufactureid = 0;
	uint32_t oui;
	uint32_t operator_id;

	hwversion = GxDLP_Get_HardwareVersion();
	swversion = GxDLP_Get_SoftwareVersion();
	manufactureid = GxDLP_Get_ManufactureId();
	if (print_version) {
		gxlogi("current STB hwversion : %s\n",convert_version_num_to_char(hwversion));
		gxlogi("            swversion : %s\n",convert_version_num_to_char(swversion));
		gxlogi("            manufactureid : %s\n",convert_version_num_to_char(manufactureid));
		gxlogi("update hwversion : %s\n",convert_version_num_to_char(image->hwver));
		gxlogi("       swversion : %s\n",convert_version_num_to_char(image->swver));
		gxlogi("       manufactureid : %s\n",convert_version_num_to_char(image->mid));
	} else {
		gxlogd("current STB hwversion : %s\n",convert_version_num_to_char(hwversion));
		gxlogd("            swversion : %s\n",convert_version_num_to_char(swversion));
		gxlogd("            manufactureid : %s\n",convert_version_num_to_char(manufactureid));
		gxlogd("update hwversion : %s\n",convert_version_num_to_char(image->hwver));
		gxlogd("       swversion : %s\n",convert_version_num_to_char(image->swver));
		gxlogd("       manufactureid : %s\n",convert_version_num_to_char(image->mid));
	}

	if (GxDLP_Get_OUI(&oui) == 0) {
		if (oui != image->oui) {
			gxloge("oui is not equal to oui in oem\n");
			return false;
		}
	}
	if (GxDLP_Get_OperatorID(&operator_id) == 0) {
		if (operator_id != image->operator_id) {
			gxloge("operator_id is not equal to operator_id in oem\n");
			return false;
		}
	}

	if (hwversion == image->hwver && manufactureid == image->mid) {
		if (image->upgrade_mode == GX_OTA_NORMAL_UPGRADE
		    || image->upgrade_mode == GX_OTA_SN_UPGRADE) {
			if (swversion < image->swver)
				return true;
		} else if (image->upgrade_mode == GX_OTA_FORCE_UPGRADE)
			return true;
		else if (image->upgrade_mode == GX_OTA_VERSION_NOTEQ_UPGRADE) {
			if (swversion != image->swver)
				return true;
		}
	}

	return false;
}

static int check_image_data_crc32(unsigned char *image_data, GxPartitionTable *image_data_table)
{
	uint32_t crc32 = 0;
	int i = 0;

	for (i = 1; i < image_data_table->count; i++) {
		if (image_data_table->tables[i].crc32_enable) {
			crc32 = GxCore_Crc32(0, (uint8_t*)(image_data + image_data_table->tables[i].start_addr), image_data_table->tables[i].used_size);
			if (crc32 != image_data_table->tables[i].crc_verify) {
				update_progress_error(GXUPDATE_IMAGE_CRC_ERROR);
				gxloge("image data partition %s check crc failed\n", image_data_table->tables[i].name);
				return -1;
			}
		}
	}

	return 0;
}

static int parse_data_header(struct download_data *ddata)
{
	uint32_t image_header_crc32 = 0;
	uint32_t image_crc32 = 0;
	struct update_image_header image_header;

	if (ddata == NULL)
		return -1;

	if (ddata->len <= UPDATE_IMAGE_HEADER_LEN) {
		gxloge("update image <  %s \n", UPDATE_IMAGE_HEADER_LEN);
		return -1;
	}
	memcpy(&image_header, ddata->data, sizeof(image_header) - 4);
	memcpy(&image_header.crc32, &ddata->data[UPDATE_IMAGE_HEADER_LEN - 4], 4);

	if (!(image_header.magicnum1 == 0xEBD88320 && image_header.magicnum2 == 0x12477CDF)) {
		update_progress_error(GXUPDATE_IMAGE_HEADER_ERROR);
		gxloge("header magicnum error\n");
		return -1;
	}

	/* TODO check image_crc32 and crc32 */
	image_header_crc32 = GxCore_Crc32(0, &ddata->data[8], UPDATE_IMAGE_HEADER_LEN - 8 - 4);
	if (image_header_crc32 != image_header.crc32) {
		update_progress_error(GXUPDATE_IMAGE_CRC_ERROR);
		gxloge("image header crc32 check error\n");
		return -1;
	}
	image_crc32 = GxCore_Crc32(0, &ddata->data[UPDATE_IMAGE_HEADER_LEN], ddata->len - UPDATE_IMAGE_HEADER_LEN);
	if (image_crc32 != image_header.image_crc32) {
		update_progress_error(GXUPDATE_IMAGE_CRC_ERROR);
		gxloge("image crc32 check error\n");
		return -1;
	}

	image_info.hwver = image_header.hwversion;
	image_info.swver = image_header.swversion;
	image_info.mid = image_header.mid;
	image_info.upgrade_mode = image_header.ota_type;
	image_info.sn_start = image_header.sn_start;
	image_info.sn_end = image_header.sn_end;

	if ((image_version_is_ok(&image_info, 1) == true) || GxDLP_Get_ForceUpgradeFLag())
	{
		GxDLP_Set_UpdateSoftVersion((char *)convert_version_num_to_char(image_header.swversion));
		GxDLP_Sync();
		return 0;
	} else {
		update_progress_error(GXUPDATE_IMAGE_VERSION_ERROR);
		gxloge("update file version error or update file sw version is older than current STB sw version");
	}

	return -1;
}

static int check_merge_bin_data(unsigned char *image_data, GxPartitionTable *ddata_table, struct merge_sub_file_s* sub_file)
{
	uint32_t crc32 = 0;
	int i = 0;
	int sub_file_index = 0;

	for (i = 0; i < ddata_table->count; i++) {
		if (mem_table.tables[i].used_size != 0)
			sub_file_index++;
		if (strcasecmp(ddata_table->tables[i].name, "TABLE") == 0)
			continue;
		if (ddata_table->tables[i].crc32_enable) {
			crc32 = GxCore_Crc32(0, (uint8_t*)(sub_file[sub_file_index].start_addr + image_data), ddata_table->tables[i].used_size);
			if (crc32 != ddata_table->tables[i].crc_verify) {
				gxloge("check upgrade data partition %s crc32e error! %s,%d\n", ddata_table->tables[i].name, __func__, __LINE__);
				return -1;
			}
		}
	}

	return 0;
}

bool is_upgrade_all_bin(void)
{
	return upgrade_all_bin;
}

struct partition *get_new_partition(void)
{
	return &mem_table;
}

int parse_data(struct download_data *ddata,struct upgrade_data *udata, int max_num)
{
	int i = 0;
	int old_cnt = 0;
	int find_part = 0;
	int need_update=0;
	unsigned char update_tag = 0;
	unsigned char *image_data;
	unsigned int image_len __attribute__((unused));
	struct merge_head_s merge_head;
	bool is_merge_bin = false;
	struct merge_sub_file_s *p_sub_file = NULL;
	unsigned int valid_data_offset = 0;
	int sub_file_index = 0;
	enum GXPARTITION_FLASH_TYPE partition_flash_type = GXPARTITION_NORFLASH;
	struct partition *origin_mem_table = NULL;
	int write_back_count = 0;

	GxCore_PartitionGetFlashType(&partition_flash_type);

	upgrade_all_bin = false;
	if (parse_data_header(ddata) != 0) {
		need_update = 0;
		goto parse_exit;
	}

	image_data = ddata->data + UPDATE_IMAGE_HEADER_LEN;
	image_len = ddata->len - UPDATE_IMAGE_HEADER_LEN;

	memcpy(&merge_head, image_data, sizeof(merge_head));

	if (strncmp(merge_head.magic, MERGE_HEAD_MAGIC, MERGE_HEAD_MAGIC_SIZE) == 0)
		is_merge_bin = true;

	if (is_merge_bin) {
		p_sub_file = (struct merge_sub_file_s *)(image_data + sizeof(merge_head));
		valid_data_offset  = sizeof(merge_head) + sizeof(struct merge_sub_file_s) * merge_head.sub_file_num + p_sub_file[0].len;
		image_data += valid_data_offset;
		for (i = 1; i < merge_head.sub_file_num; i++) {
			if (p_sub_file[i].start_addr < valid_data_offset) {
				gxloge("[Downloader] merge bin error, please check merge bin, %s, %d\n");
				need_update = 0;
				goto parse_exit;
			}
			p_sub_file[i].start_addr -= valid_data_offset;
		}
	}

	//获取原有partition表
	flash_table = GxCore_PartitionFlashInit();
	if(flash_table == NULL) {
		gxloge("[Downloader] GxOem_PartitionTableGet  old Error!\n");
		need_update = 0;
		goto parse_exit;
	}

	/*利用patition结构格式化info信息*/
	if (GxCore_MEM_Partition_Init((char*)image_data,&mem_table) != 0) {
		gxloge("[Downloader] GxOem_PartitionTableGet0	New  Error!\n");
		need_update = 0;
		goto parse_exit;
	}
	//以下对两部分partition 表进行比对，获取需要更新的部分
	gxlogd("Flash Partition:");
	GxCore_PartitionPrint(flash_table);
	gxlogd("Mem Partition:");
	GxCore_PartitionPrint(&mem_table);

	ota_img.image_data = NULL;
	ota_img.image_size = 0;
	if (is_merge_bin == true) {
		if (check_merge_bin_data(image_data, &mem_table, p_sub_file) != 0) {
			need_update = 0;
			goto parse_exit;
		}
		for (i = 0; i < mem_table.count; i++) {
			if(strcmp(mem_table.tables[i].name, "TABLE") == 0) {
				update_tag = mem_table.tables[i].update_tags;
				break;
			}
		}

		if (i == mem_table.count) {
			need_update = 0;
			goto parse_exit;
		}
	} else {
		if (check_image_data_crc32(image_data, &mem_table) != 0) {
			need_update = 0;
			goto parse_exit;
		}

		if(strcasecmp(mem_table.tables[0].name, "TABLE") == 0)
			update_tag = mem_table.tables[0].update_tags;
		else {
			gxloge("first partition of update bin must be partition TABLE\n");
			need_update = 0;
			goto parse_exit;
		}
	}

	/*table 第一个字段标志为１代表强行升级整个bin，在这种情况只有两个分区，table + update*/
	if (update_tag == 1) {
		upgrade_all_bin = true;
		if (is_merge_bin != true) {
			image_data += mem_table.tables[0].total_size;
			if (mem_table.count > 2) {
				origin_mem_table = GxCore_Malloc(sizeof(struct partition));
				if (origin_mem_table == NULL) {
					gxloge("malloc origin_mem_table failed\n");
					need_update = 0;
					goto parse_exit;
				}
				memcpy(origin_mem_table, &mem_table, sizeof(struct partition));
			}
			memset(&mem_table, 0x00, sizeof(struct partition));
			/* parse all bin */
			if (GxCore_MEM_Partition_Init((char*)(image_data),&mem_table) != 0) {
				gxloge("Get new all bin partition error\n");
				need_update = 0;
				goto parse_exit;
			}
			gxlogd("new all bin partition: \n");
			GxCore_PartitionPrint(&mem_table);
		}
	}

	for(i = 0; i < mem_table.count; i++) {
		if (is_merge_bin == true && mem_table.tables[i].used_size != 0)
			sub_file_index++;
		if (upgrade_all_bin == true) {
			//目标地址是原有分段的起始地址
			udata[need_update].dst = mem_table.tables[i].start_addr;

			//源地址是buffer基地址加上偏移量
			if (is_merge_bin == true)
				udata[need_update].src = p_sub_file[sub_file_index].start_addr + image_data;
			else
				udata[need_update].src = mem_table.tables[i].start_addr + image_data;
			//大小是新的表中的已用大小
			if (partition_flash_type == GXPARTITION_NORFLASH)
				udata[need_update].size = mem_table.tables[i].total_size;
			else
				udata[need_update].size = mem_table.tables[i].used_size;
			strncpy(udata[need_update].name,mem_table.tables[i].name,10);
			udata[need_update].partition_pos = i;
			udata[need_update].partition = &mem_table.tables[i];
			udata[need_update].write_back = false;
			if (origin_mem_table) {
				int j;

				/* get write back partition */
				for (j = 2; j < origin_mem_table->count; j++) {
					if (!strncasecmp(mem_table.tables[i].name, origin_mem_table->tables[j].name, FLASH_PARTITION_NAME_SIZE)
					    && origin_mem_table->tables[j].update_tags == 3) {
						udata[need_update].write_back = true;
						write_back_count++;
						break;
					}
				}
			}
			/* check write back partition is suitable for origin
			 * partition in flash */
			if (udata[need_update].write_back) {
				for (old_cnt = 0; old_cnt < flash_table->count; old_cnt++) {
					if (strcasecmp(mem_table.tables[i].name, flash_table->tables[old_cnt].name) == 0)
						break;
				}
				if (old_cnt == flash_table->count) {
					gxloge("can't find partition %s in flash which is need write back\n", mem_table.tables[i].name);
					need_update = 0;
					goto parse_exit;
				}
				if (mem_table.tables[i].partition_size < flash_table->tables[old_cnt].partition_size) {
					gxloge("[Downloader] Size of partition %s which is need write back is smaller than size of origin partitoin in flash\n", mem_table.tables[i].name);
					need_update = 0;
					goto parse_exit;
				}
			}
			need_update++;
		} else {
			if (is_merge_bin != true)
				i = (i == 0) ? 1 : i; //skip first partition "TABLE"
			find_part = 0;
			if (strncasecmp(mem_table.tables[i].name, "BOOT", FLASH_PARTITION_NAME_SIZE) == 0
				|| strncasecmp(mem_table.tables[i].name, "TABLE", FLASH_PARTITION_NAME_SIZE) == 0)
				continue;

			if (strncasecmp(mem_table.tables[i].name, "OTA", FLASH_PARTITION_NAME_SIZE) == 0 &&
			    mem_table.tables[i].update_tags == 0) {
				ota_img.image_size = mem_table.tables[i].used_size;
				if (is_merge_bin == true)
					ota_img.image_data = p_sub_file[sub_file_index].start_addr + image_data;
				else
					ota_img.image_data = mem_table.tables[i].start_addr + image_data;
			}
			/* 备份dlp分区不能进行升级 */
			if (strncasecmp(mem_table.tables[i].name, Ini_backoem_name(), FLASH_PARTITION_NAME_SIZE) == 0)
				continue;

			update_tag = mem_table.tables[i].update_tags;
			switch(update_tag) {
				case 2:/* update */
					break;
				default:
					gxlogd("[Downloader] filter part: %s\n",mem_table.tables[i].name);
					continue;
			}

			//对比收到数据的namne和本地table里面的
			for(old_cnt=0;old_cnt<flash_table->count;old_cnt++) {
				if(strcasecmp(mem_table.tables[i].name,flash_table->tables[old_cnt].name) == 0) {
					find_part = 1;
					gxlogd("[Downloader] Find partition: %s!\n", mem_table.tables[i].name);
					break;
				}
			}

#ifdef LINUX_OS
			if (Ini_oem_use_minifs() && strcasecmp(mem_table.tables[i].name, DATA_PARTITION_NAME) == 0) {
				gxloge("becasue oem file saved in DATA PARTITION, linux does not support UPGRADE DATA patition\n");
				need_update = 0;
				goto parse_exit;
			}
#endif

			if(find_part) {
				//目标地址是原有分段的起始地址
				udata[need_update].dst = flash_table->tables[old_cnt].start_addr;

				//源地址是buffer基地址加上偏移量
				if (is_merge_bin == true)
					udata[need_update].src = p_sub_file[sub_file_index].start_addr + image_data;
				else
					udata[need_update].src = mem_table.tables[i].start_addr + image_data;

				//比较空间是否足够,此处不能>=否则签名后升级不了。
				if(mem_table.tables[i].used_size > flash_table->tables[old_cnt].partition_size) {
					gxloge("[Downloader] New partition size %d is large than old partition total size %d->%s!\n",\
							mem_table.tables[i].used_size, flash_table->tables[old_cnt].partition_size, mem_table.tables[i].name);
					need_update = 0;
					goto parse_exit;
				}
				//大小是新的表中的已用大小
				udata[need_update].size = mem_table.tables[i].used_size;
				gxlogd("[Downloader] need update : %s!data[i].src = %p| data[i].dst  = 0x%08x |data.size = %d, crc = %x\n",\
						mem_table.tables[i].name,udata[need_update].src,udata[need_update].dst,
						udata[need_update].size,
						mem_table.tables[i].crc32);

				//13D99C1A
				strncpy(udata[need_update].name,mem_table.tables[i].name,10);

				udata[need_update].partition_pos = old_cnt;
				udata[need_update].partition = &flash_table->tables[old_cnt];

				need_update ++;
			} else {
				need_update = 0;
				goto parse_exit;
			}
		}
	}

parse_exit:
	if (origin_mem_table != NULL) {
		if (write_back_count != (origin_mem_table->count - 2)) {
			gxloge("some write back part can't find in partition table of ALL bin\n");
			need_update = 0;
		}
		GxCore_Free(origin_mem_table);
	}
	return need_update;
}
