#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_utility.h"
#include "app_wnd_file_list.h"
#include <common/gx_hw_malloc.h>
#include <gxcore.h>
#include <common/gxpm.h>
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if NETWORK_SUPPORT && GPRS_SUPPORT
#include "module/app_gprs.h"
#endif
#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
#endif

#ifdef ECOS_OS
#define FONT_RAM_PATH     "/mnt/temp.ttf"
#endif
#ifdef LINUX_OS
#define FONT_RAM_PATH    "/tmp/temp.ttf"
#endif
#define FONT_FILE_PATH    WORK_PATH"theme/font/arial.ttf"
#define FONT_RAM_NAME    "font_26_ram"

#define E_INVALID_HANDLE                    (0)
#define BIN_BLOCK_SIZE (uint32_t)(0x10000)
#define UPGRADE_SUFFIX "bin;BIN"
#define DATA_SECTION_NAME "DATA"
#define DUNM_BIN_ALL "dump_all.bin"
#define MOUNT_SECT_THEME_PATH "/home/theme_data"
#define MERGE_HEAD_MAGIC              "mergebin"
#define MERGE_HEAD_MAGIC_SIZE         8
#define FILE_NAME_MAX_LEN               256

typedef struct merge_sub_file_s {
    char name[FILE_NAME_MAX_LEN];
    uint32_t start_addr;
    uint32_t len;
}Mergesubfile;

typedef struct merge_head_s {
    char magic[MERGE_HEAD_MAGIC_SIZE];
    int sub_file_num;
}Mergehead;

enum UPGRADE_ITEM_NAME
{
	ITEM_UPGRADE_TYPE = 0,
	ITEM_UPGRADE_SECT,
	ITEM_UPGRADE_PATH,
	ITEM_UPGRADE_EXEC,
	ITEM_UPGRADE_TOTAL
};

typedef enum
{
	UPGRADE_TYPE_U2S,
	UPGRADE_TYPE_DUMP
}UpgradeTypeSel;

typedef enum
{
	UPGRADE_SECT_ALL,
	UPGRADE_SECT_APP,
	UPGRADE_SECT_DATA
}UpgradeSectSel;

typedef struct
{
	UpgradeTypeSel type;
	UpgradeSectSel section;
	char *path;
}UpgradePara;

typedef struct
{
    UpgradePara para;
    uint32_t total_size;
    uint32_t cur_size;
    UpgradeState state;
    enum GXPARTITION_FLASH_TYPE flash_type;
}UsbUpgradeInfo;

GxHwMallocObj hw_all_data;
static SystemSettingOpt s_upgrade_opt;
static SystemSettingItem s_upgrade_item[ITEM_UPGRADE_TOTAL];
static uint32_t s_flash_size = 4 * SIZE_UNIT_M;
static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;
static UsbUpgradeInfo s_upgrade_info;
static event_list* sp_UpgradeTimeOut = NULL;

static void _progress_show_error(void);
static status_t _copy_font2ramfs(void);

//#define CRC_DEBUG
#ifdef CRC_DEBUG
static char *crc32str(unsigned int crc)
{
    static char buf[9] = {0, };

    if (crc == 0)
        memset(buf, ' ', 8);
    else
        sprintf(buf, "%08X", crc);

    return buf;
}

static int app_crc_debug(struct partition *tbl)
{
    int i = 0;
    char size_str1[32], size_str2[32];
    app_log_debug("Partition Version :  %d\n", tbl->version);
    app_log_debug("Partition Count   :  %d\n", tbl->count);
    app_log_debug("Write Protect     :  %s\n", tbl->write_protect ? "TRUE": "FALSE");
    app_log_debug("CRC32 Enable      :  %s\n", tbl->crc32_enable ? "TRUE": "FALSE");
    app_log_debug("Table CRC32       :  %X\n", tbl->crc_verify);

    app_log_debug("==========================================================\n");
    app_log_debug("%-8s%-10s%-9s%10s%11s%10s\n", "NAME",  "CRC32", "START", "SIZE", "USED", "Use%");
    app_log_debug("==========================================================\n");
    for (i=0; i < tbl->count; i++) {
        app_log_debug("%-8s%-10s%08x%11s%11s%8d %%\n",
                tbl->tables[i].name,
                crc32str(tbl->tables[i].crc_verify),
                tbl->tables[i].start_addr,
                get_size_str(size_str1, tbl->tables[i].total_size),
                get_size_str(size_str2, tbl->tables[i].used_size),
                tbl->tables[i].total_size > 0 ? tbl->tables[i].used_size * 100 / tbl->tables[i].total_size : 0
              );
    }
    app_log_debug("----------------------------------------------------------\n\n");
    return 0;
}

#endif

//將字库导入内存
void app_copy_font_to_ram(void)
{
    int font_ram_size = 0;

    _copy_font2ramfs();
    if(APP_THEME_CLASSIC_BLUE)
        font_ram_size = 26;
    else if(APP_THEME_CLASSIC_PURPLE || APP_THEME_GRID_PURPLE)
        font_ram_size = 20;
    else
        font_ram_size = 21;
    GXGDI_FontLoad((const unsigned char *)FONT_RAM_NAME, FONT_RAM_PATH, font_ram_size, 0);
}

void app_font_ram_destroy(void)
{
    int init_value = 0;
    GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
    GUI_SetInterface("osd_language", g_LangName[init_value]);
    GXGDI_FontDestroy((const unsigned char *)FONT_RAM_NAME);
}

int app_get_update_state(void)
{
    return s_upgrade_info.state;
}

int app_upgrade_crc_check(char *path)
{
    struct partition tbl;
    int ret = 0;
    FILE *file = NULL;

    if(GxCore_FILE_PartitionInit(&tbl, path))
    {
        file = fopen(path, "r");
        if (file == 0)
        {
            app_log_error("----------file open err.\n");
            return 0;
        }
        #ifdef CRC_DEBUG
            app_crc_debug(&tbl);
        #endif
        if(tbl.crc32_enable > 0)
        {
            int i = 0;
            unsigned int new_crc;

            memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
            hw_all_data.size = s_flash_size;
            GxCore_HwMalloc(&hw_all_data,MALLOC_WITH_CACHE);
            if(hw_all_data.usr_p == NULL)
            {
                fclose(file);
                return 0;
            }

            for(i = 0; i < tbl.count; i++)
            {
                if(tbl.tables[i].crc32_enable != 0)
                {
                    fseek(file, tbl.tables[i].start_addr, SEEK_SET);
                    if(tbl.tables[i].start_addr+tbl.tables[i].total_size >  hw_all_data.size)
                    {
                        app_log_error("(1)------flash size is larger.\n");
                        goto END;
                    }
                    ret = fread((unsigned char *)hw_all_data.usr_p+tbl.tables[i].start_addr, 1, tbl.tables[i].total_size, file);
                    if (ret !=  tbl.tables[i].total_size)
                    {
                        app_log_error("(1)------read failed %d.\n", ret);
                        goto END;
                    }

                    //genflash don't support crc check for partition [TABLE]
                    if(strncasecmp(tbl.tables[i].name, PARTITION_TABLE, strlen(PARTITION_TABLE)) == 0)
                        continue;

                    new_crc = GxCore_Crc32(0, (unsigned char *)hw_all_data.usr_p+tbl.tables[i].start_addr,  tbl.tables[i].used_size);
#ifdef CRC_DEBUG
                    app_log_debug("------0x%X----------------------0x%X-----------\n\n", new_crc, tbl.tables[i].crc_verify);
#endif
                    if(new_crc != tbl.tables[i].crc_verify)
                        goto END;
                }
                else
                {
                    fseek(file, tbl.tables[i].start_addr, SEEK_SET);
                    if(tbl.tables[i].start_addr+tbl.tables[i].total_size >  hw_all_data.size)
                    {
                        app_log_error("(1)------flash size is larger.\n");
                        goto END;
                    }
                    ret = fread((unsigned char *)hw_all_data.usr_p+tbl.tables[i].start_addr, 1, tbl.tables[i].total_size, file);
                    if (ret !=  tbl.tables[i].total_size)
                    {
                        app_log_error("(2)---**---read failed %d.\n", ret);
                        goto END;
                    }
                }
            }
        }
        fclose(file);
        return 1;
    }
    else
    {
        return 0;
    }

END:
    fclose(file);
    GxCore_HwFree(&hw_all_data);
    memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    ret =0;
    return ret;
}

int app_upgrade_nand_crc_check(char *path)
{
    GxFileInfo info;
    handle_t file = 0;
    unsigned char *image_data = NULL;
    Mergehead merge_head;
    Mergesubfile *p_sub_file = NULL;
    unsigned int valid_data_offset = 0;
    int sub_file_index = 0;
    uint32_t crc32 = 0;
    int i = 0;
    struct partition mem_table = {0};

    memset(&info,0,sizeof(GxFileInfo));
    GxCore_GetFileInfo(path,&info);

    if((file = GxCore_Open(path, "r")) < 0)
    {
        app_log_error("----------file open err.\n");
        return 0;
    }
    memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    hw_all_data.size = info.size_by_bytes;
    GxCore_HwMalloc(&hw_all_data,MALLOC_WITH_CACHE);
    if(hw_all_data.usr_p == NULL)
    {
        GxCore_Close(file);
        return 0;
    }

    if(GxCore_Read(file,(unsigned char *)hw_all_data.usr_p, 1, info.size_by_bytes) <= 0)
        goto END;

    image_data = hw_all_data.usr_p;

    memcpy(&merge_head, image_data, sizeof(merge_head));

    if (strncmp(merge_head.magic, MERGE_HEAD_MAGIC, MERGE_HEAD_MAGIC_SIZE) != 0)
        goto END;

    p_sub_file = (Mergesubfile *)(image_data + sizeof(merge_head));
    valid_data_offset  = sizeof(merge_head) + sizeof(Mergesubfile) * merge_head.sub_file_num + p_sub_file[0].len;
    image_data += valid_data_offset;
    for (i = 1; i < merge_head.sub_file_num; i++) {
        if (p_sub_file[i].start_addr < valid_data_offset) {
            goto END;
        }
        p_sub_file[i].start_addr -= valid_data_offset;
    }

    /*利用patition结构格式化info信息*/
    if (GxCore_MEM_Partition_Init((char*)image_data,&mem_table) != 0)
        goto END;


   for (i = 0; i < mem_table.count; i++) {
       if (mem_table.tables[i].used_size != 0)
        {
           s_upgrade_info.total_size += mem_table.tables[i].used_size;
           sub_file_index++;
        }
       if (strcasecmp(mem_table.tables[i].name, "TABLE") == 0)
           continue;
       if (mem_table.tables[i].crc32_enable) {
           crc32 = GxCore_Crc32(0, (uint8_t*)(p_sub_file[sub_file_index].start_addr + image_data), mem_table.tables[i].used_size);
           if (crc32 != mem_table.tables[i].crc_verify) {
               goto END;
           }
       }
   }
   GxCore_Close(file);
   return 1;
END:
    GxCore_Close(file);
    GxCore_HwFree(&hw_all_data);
    memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    return 0;
}

static bool _check_upgrade_file_valid(UpgradePara *ret_para)
{
	bool ret = true;
    if(ret_para->path == NULL)
        return false;
    if(s_upgrade_info.flash_type == GXPARTITION_NORFLASH)
    {
        if(app_upgrade_crc_check(ret_para->path) == 0)
            return false;
    }
    else
    {
        if(app_upgrade_nand_crc_check(ret_para->path) == 0)
            return false;
    }
    return ret;
}

static status_t _copy_font2ramfs(void)
{
    handle_t font_src = E_INVALID_HANDLE;
    handle_t font_dest = E_INVALID_HANDLE;
    GxFileInfo font_file_info;
    status_t ret = GXCORE_ERROR;
    char *buf = NULL;
    int i = 0;

    if(GxCore_FileExists(FONT_FILE_PATH) == GXCORE_FILE_UNEXIST)
        return GXCORE_ERROR;

    if(GxCore_FileExists(FONT_RAM_PATH) == GXCORE_FILE_EXIST)
        return GXCORE_SUCCESS;

    if((buf = (char*)GxCore_Calloc(SIZE_UNIT_K, 1)) == NULL)
        return GXCORE_ERROR;

    if((font_src = GxCore_Open(FONT_FILE_PATH, "r")) < 0)
    {
        GxCore_Free(buf);
        return GXCORE_ERROR;
    }

    if((font_dest = GxCore_Open(FONT_RAM_PATH, "w+")) < 0)
    {
        GxCore_Free(buf);
        GxCore_Close(font_src);
        return GXCORE_ERROR;
    }

    memset(&font_file_info, 0, sizeof(GxFileInfo));
    GxCore_GetFileInfo(FONT_FILE_PATH, &font_file_info);

    for(i = 0; i < font_file_info.size_by_bytes; i += SIZE_UNIT_K)
    {
        memset(buf, 0, SIZE_UNIT_K);
        ret = GxCore_Read(font_src, (void*)buf, 1, SIZE_UNIT_K);
        if(ret <= 0)
            break;
        ret = GxCore_Write(font_dest, (void*)buf, 1, ret);
        if(ret <= 0)
            break;
    }

    GxCore_Free(buf);
    GxCore_Close(font_src);
    GxCore_Close(font_dest);

    if(ret > 0)
        return GXCORE_SUCCESS;
    else
        return GXCORE_ERROR;
}

static GxHwMallocObj hw_load_firmware = {0};
static char s_upgade_gpio_powercut[32] ;
static void app_upgrade_lowpower_free_firmware(void)
{
    if (hw_load_firmware.usr_p != NULL)
    {
        GxCore_HwFree(&hw_load_firmware);
        memset(&hw_load_firmware, 0, sizeof(GxHwMallocObj));
    }
}

static int app_upgrade_lowpower_load_firmware(void)
{
#define LOWPOWER_FIRMWARE_PATH "/lib/firmware/gxlowpower.fw"
    int length = 0;
    FILE *fp = NULL;
    int ret ;

    memset(s_upgade_gpio_powercut, 0, sizeof(s_upgade_gpio_powercut));
    if(GXCORE_SUCCESS != app_get_gpio_config(GPIO_POWERCUT,s_upgade_gpio_powercut,sizeof(s_upgade_gpio_powercut)))
    {
        app_log_error("app_upgrade_lowpower_load_firmware gpio failer \n");
        return -1;
    }

    memset(&hw_load_firmware, 0, sizeof(GxHwMallocObj));

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(LOWPOWER_FIRMWARE_PATH))
    {
        app_log_error("app_upgrade_lowpower_load_firmware not exist \n");
        return -1;
    }
    fp = fopen(LOWPOWER_FIRMWARE_PATH, "r");
    if(NULL == fp)
    {
        app_log_error("app_upgrade_lowpower_load_firmware open failer \n");
        return -1;
    }
    ret = fseek(fp, 0, SEEK_END);
    if(0 != ret)
    {
        app_log_error("app_upgrade_lowpower_load_firmware fseek failer \n");
        fclose(fp);
        return -1;
    }
    length = ftell(fp);
    if(length <= 0 )
    {
        app_log_error("app_upgrade_lowpower_load_firmware ftell failer \n");
        fclose(fp);
        return -1;
    }
    ret = fseek(fp, 0, SEEK_SET);
    if(0 != ret)
    {
        app_log_error("app_upgrade_lowpower_load_firmware fseek failer \n");
        fclose(fp);
        return -1;
    }

    hw_load_firmware.size = length;
    GxCore_HwMalloc(&hw_load_firmware,MALLOC_WITH_CACHE);
    if(hw_load_firmware.usr_p == NULL)
    {
        app_log_error("app_upgrade_lowpower_load_firmware GxCore_HwMalloc failer \n");
        fclose(fp);
        return -1;
    }

    if (length != fread(hw_load_firmware.usr_p, 1, length, fp))
    {
        app_log_error("app_upgrade_lowpower_load_firmware fread failer \n");
        app_upgrade_lowpower_free_firmware();
        fclose(fp);
        return -1;
    }
    fclose(fp);

    app_log_debug("app_upgrade_load len=%d, s=%s \n",length,s_upgade_gpio_powercut);

    return 0;
}

static void app_upgrade_reboot(void)
{
    struct lowpower_firmware fw;
    struct lowpower_info_s lowpower;
#define MAX_LOWPOWER_CMD_LEN 512
    char* cmd = NULL;

    cmd = GxCore_Calloc(1, MAX_LOWPOWER_CMD_LEN);
    if( (cmd != NULL) && (hw_load_firmware.usr_p != NULL) )
    {
        fw.data = hw_load_firmware.usr_p;
        fw.size = hw_load_firmware.size;

        lowpower.WakeTime = 1;
        lowpower.GpioMask = 0;
        lowpower.GpioData = 0;
        lowpower.key = 0x00;
        sprintf(cmd, "keys=0xbfaf,0xbbaf,0x47 powercut=%s",s_upgade_gpio_powercut);
        app_log_debug("app_upgrade cmd = %s \n",cmd);
        lowpower.cmdline = (unsigned char*)cmd;

        GxLowpower_LoadFirmware(&fw);
        GxLowpower_PassParam(&lowpower);
        GxLowpower_Enter();
        return ;
    }

    app_log_error("app_upgrade switch reboot\n");
    GxCore_Reboot();
}

static void app_upgrade_para_get(UpgradePara *ret_para)
{
    ret_para->type= s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;
    ret_para->section= s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel;

    if((strcmp(s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string, STR_ID_PRESS_OK) == 0)
            || (strcmp(s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string, "") == 0))
    {
        ret_para->path = NULL;
    }
    else
    {
        ret_para->path = s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string;
    }
}

void app_upgrade_clear_path(void)
{
	 s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	 app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}
	s_file_path = NULL;
    app_update_pop_dlg(WND_UPGRADE_PROCESS);
    app_update_pop_dlg(WND_FILE_LIST);
    app_update_pop_dlg(WND_POP_BOOK);
}

char *app_upgrade_get_path(void)
{
	return s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string;
}

static int app_upgrade_exit_callback(ExitType exit_type)
{
	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}

	s_file_path = NULL;

	app_fuse_spec_epg_start_work();

	return EXIT_RET_DEFAULT;
}

static int app_upgrade_type_change_callback(int sel)
{
	s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = 0;
	app_system_set_item_property(ITEM_UPGRADE_SECT, &(s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty));

	if(sel == UPGRADE_TYPE_U2S)
	{
        app_system_set_item_state_update(ITEM_UPGRADE_SECT, ITEM_NORMAL);
	}
	else if(sel == UPGRADE_TYPE_DUMP)
	{
        app_system_set_item_state_update(ITEM_UPGRADE_SECT, ITEM_DISABLE);
	}
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel = sel;
	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}
	s_file_path =NULL;
	return EVENT_TRANSFER_STOP;
}

static int app_upgrade_section_change_callback(int sel)
{
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = sel;
	return EVENT_TRANSFER_STOP;
}

static int _app_upgrade_path_filedlg_exit_cb(WndStatus ret)
{
    if(ret == WND_OK)
    {
        app_get_file_real_path(&s_file_path);

        if(s_file_path != NULL)
        {
            int str_len = 0;

            s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = s_file_path;
            app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);

            if(s_file_path_bak != NULL)
            {
                GxCore_Free(s_file_path_bak);
                s_file_path_bak = NULL;
            }

            str_len = strlen(s_file_path);
            s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if(s_file_path_bak != NULL)
            {
                memcpy(s_file_path_bak, s_file_path, str_len);
                s_file_path_bak[str_len] = '\0';
            }
        }
    }
    else
    {
        if(s_file_path_bak)
            s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = s_file_path_bak;
        else
            s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
        app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
    }
    app_update_pop_dlg(WND_POP_BOOK);

    return 0;
}

static int app_upgrade_path_button_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        UpgradeTypeSel type = s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;

        if(check_usb_status() == false)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            pop.pos.x = APP_POP_DLG_POS_X;
            pop.pos.y = APP_POP_DLG_POS_Y;
            popdlg_create(&pop);
        }
        else
        {
            FileListParam file_para;
            memset(&file_para, 0, sizeof(file_para));

            file_para.cur_path = s_file_path_bak;
            file_para.dest_path = &s_file_path;
            file_para.suffix = UPGRADE_SUFFIX;
            file_para.pos_x = APP_WND_FILE_LIST_POS_X;
            file_para.pos_y = APP_WND_FILE_LIST_POS_Y;
            file_para.exit_cb = _app_upgrade_path_filedlg_exit_cb;
            if(type == UPGRADE_TYPE_U2S)
            {
                file_para.dest_mode = DEST_MODE_FILE;
                app_get_file_path_dlg(&file_para);
            }
            else if(type == UPGRADE_TYPE_DUMP)
            {
                file_para.dest_mode = DEST_MODE_DIR;
                app_get_file_path_dlg(&file_para);
            }
        }
    }

	s_file_path =NULL;

    return EVENT_TRANSFER_KEEPON;
}

static void _usb_dump_exec(void *arg)
{
#define DATA_WRITE_LENGTH 8*1024
    int i = 0;
    struct partition *flash_table = NULL;
    struct partition_info* partition = NULL;
    char path[100] = {0};
    char* buf = NULL;
    int size = 0;
    int real_size = 0;
    handle_t fd;

    sprintf(path,"%s/%s",s_upgrade_info.para.path,DUNM_BIN_ALL);

    flash_table = GxCore_PartitionFlashInit();
    s_upgrade_info.total_size = s_flash_size;
    fd = GxCore_Open(path,"w");
    if(fd < 0)
    {
        s_upgrade_info.state = UPGRADE_STOPPED;
        return;
    }
    buf = GxCore_Malloc(DATA_WRITE_LENGTH);
    if(buf == NULL)
    {
        app_log_error("Malloc readbuf errot\n");
        return;
    }
    for(i = 0; i < flash_table->count; i++)
    {
        partition = &flash_table->tables[i];
        size = 0;
        while(size < flash_table->tables[i].total_size)
        {
            memset(buf,0,DATA_WRITE_LENGTH);
            if(flash_table->tables[i].total_size - size < DATA_WRITE_LENGTH)
                real_size = flash_table->tables[i].total_size - size;
            else
                real_size = DATA_WRITE_LENGTH;
            GxCore_PartitionRead(partition,buf,real_size,size);
            if(GxCore_Write(fd,buf,1,real_size) <= 0)
            {
                s_upgrade_info.state = UPGRADE_STOPPED;
                goto err;
            }
            s_upgrade_info.cur_size += real_size;
            size += real_size;
            GxCore_ThreadDelay(10);
        }
    }
    s_upgrade_info.state = UPGRADE_SUCCESS;
err:
    GxCore_Free(buf);
    buf = NULL;
    GxCore_Close(fd);
    return;
}

int app_usb_dump(UpgradePara *upgrade_para)
{
    int thread_id = 0;
    memcpy(&(s_upgrade_info.para),upgrade_para,sizeof(UpgradePara));
    GxCore_ThreadCreate("flash_dump_data", &thread_id, _usb_dump_exec, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
    return 0;
}

static status_t _upgrade_usb_dump_exec(UpgradePara *dump_para)
{
    status_t ret = GXCORE_ERROR;
    char *partiton = NULL;
    PopDlg  pop = {0};

    if(dump_para == NULL)
        return GXCORE_ERROR;

    partiton = get_usb_url_partition(dump_para->path);
    if(partiton != NULL)
    {
        if(get_usb_partition_space(partiton) <= s_flash_size)
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_NO_SPACE;
            pop.mode = POP_MODE_UNBLOCK;
            pop.pos.x = APP_POP_DLG_POS_X;
            pop.pos.y = APP_POP_DLG_POS_Y;
            popdlg_create(&pop);
        }
        else
        {
            memset(&s_upgrade_info,0,sizeof(UsbUpgradeInfo));
            GUI_CreateDialog(WND_UPGRADE_PROCESS);
            app_usb_dump(dump_para);
        }

        GxCore_Free(partiton);
        partiton = NULL;
    }
    return ret;
}

static int _check_sections(UpgradePara *para)
{
    unsigned char *image_data = NULL;
    struct partition *flash_table = NULL;
    struct partition_info* src_part = NULL;
    struct partition_info* dst_part = NULL;

    struct partition mem_table = {0};
    image_data = hw_all_data.usr_p;
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        app_log_error("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        return -1;
    }
    flash_table = GxCore_PartitionFlashInit();

    if (para->section == UPGRADE_SECT_APP  || para->section == UPGRADE_SECT_DATA)
    {
        dst_part = app_get_partition(flash_table, DATA_SECTION_NAME);
        src_part = app_get_partition(&mem_table, DATA_SECTION_NAME);
        if(dst_part == NULL || src_part == NULL)
            return -1;
        if (para->section == UPGRADE_SECT_APP && dst_part->start_addr == src_part->start_addr)
            return 0;
        if(para->section == UPGRADE_SECT_DATA && dst_part->total_size >= src_part->used_size)
            return 0;
        return -1;
    }

    return 0;
}

static int _usb_upgrade_partition_exec(unsigned char *buf, int size, struct partition_info *partition)
{
    int ret = 0;
    int ssize = 0;
    int write_real_size = 0;
    unsigned int flash_block_size = 0;
    uint32_t last_size = 0;

    if((buf == NULL) || (partition == NULL) || (size == 0))
    {
        app_log_error("para err!!!\n");
        return -1;
    }

    ret = GxCore_PartitionGetFlashBlockSize(&flash_block_size);
    if ( (ret != 0) || (flash_block_size <=0 ) )
    {
        app_log_error("GetFlashBlockSize err %d \n",flash_block_size);
        return -1;
    }
    last_size = s_upgrade_info.cur_size + partition->total_size;

    GxCore_Partition_Unprotect();
    ret = GxCore_PartitionErase(partition);
    if (ret != 1)
    {
        app_log_error("Partition Erase err\n");
        return -1;
    }
    ssize = 0;
    write_real_size = 0;

    while (ssize < size)
    {
        write_real_size = ((size - ssize) >= flash_block_size ? flash_block_size : (size - ssize));
        ret = GxCore_PartitionWriteRaw(partition, (void *)(buf+ssize), write_real_size, ssize);
        if (ret != 1)
        {
            app_log_error("Partition write err \n");
            return -1;
        }
        ssize += write_real_size;
        s_upgrade_info.cur_size += write_real_size;
        GxCore_ThreadDelay(20);
    }
    s_upgrade_info.cur_size = last_size;
    return 0;
}

static void _usb_upgrade_nor_exec(void *arg)
{
    int i = 0;
    struct partition mem_table = {0};
    struct partition *flash_table = NULL;
    unsigned char *image_data = NULL;
    struct partition_info* partition = NULL;
    int ret = 0;

    image_data = hw_all_data.usr_p;
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        app_log_error("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        goto err;
    }
    if (UPGRADE_SECT_APP == s_upgrade_info.para.section)
    {
        flash_table = GxCore_PartitionFlashInit();
        partition = GxCore_PartitionGetByName(flash_table,DATA_SECTION_NAME);
        s_upgrade_info.total_size = s_flash_size - partition->total_size;
        for(i = 0; i < mem_table.count; i++)
        {
            partition = &mem_table.tables[i];
            if((strcasecmp(mem_table.tables[i].name, DATA_SECTION_NAME) != 0))
            {
                ret = _usb_upgrade_partition_exec(image_data + mem_table.tables[i].start_addr, partition->total_size, partition);
                if(ret == -1)
                    goto err;
            }
        }
    }
    else if(UPGRADE_SECT_DATA == s_upgrade_info.para.section)
    {
        flash_table = GxCore_PartitionFlashInit();
        partition = GxCore_PartitionGetByName(flash_table,DATA_SECTION_NAME);
        if(partition != NULL)
        {
            s_upgrade_info.total_size = partition->total_size;
            for(i=0; mem_table.count; i++)
            {
                if((strcasecmp(mem_table.tables[i].name, DATA_SECTION_NAME) == 0))
                {
                    ret = _usb_upgrade_partition_exec(image_data + mem_table.tables[i].start_addr, mem_table.tables[i].total_size, partition);
                    if(ret == -1)
                        goto err;
                    break;
                }
            }
        }
    }
    else
    {
        s_upgrade_info.total_size = s_flash_size;
        for(i = 0; i < mem_table.count; i++)
        {
            partition = &mem_table.tables[i];
            ret = _usb_upgrade_partition_exec(image_data + mem_table.tables[i].start_addr, partition->total_size, partition);
            if(ret == -1)
                goto err;
        }
    }
    s_upgrade_info.state = UPGRADE_SUCCESS;
    app_log_min("Upgrade Success \n");
    return;
err:
    s_upgrade_info.state = UPGRADE_STOPPED;
    if(hw_all_data.usr_p != NULL)
    {
        GxCore_HwFree((void*)&hw_all_data.usr_p);
        memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    }
    app_log_error("Upgrade Fail \n");
    return;
}

int app_usb_nor_upgrade(UpgradePara *upgrade_para)
{
    int thread_id = 0;
    memcpy(&(s_upgrade_info.para),upgrade_para,sizeof(UpgradePara));
    GxCore_ThreadCreate("nor_update_data", &thread_id, _usb_upgrade_nor_exec, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
    return 0;
}

static void _usb_upgrade_nand_exec(void *arg)
{
    int i = 0;
    struct partition mem_table = {0};
    struct partition *flash_table = NULL;
    unsigned char *image_data = NULL;
    struct partition_info* partition = NULL;
    Mergehead merge_head;
    Mergesubfile *p_sub_file = NULL;
    unsigned int valid_data_offset = 0;
    int sub_file_index = 0;
    int ret = 0;

    image_data = hw_all_data.usr_p;
    memcpy(&merge_head, image_data, sizeof(merge_head));
    p_sub_file = (Mergesubfile *)(image_data + sizeof(merge_head));
    valid_data_offset  = sizeof(merge_head) + sizeof(Mergesubfile) * merge_head.sub_file_num + p_sub_file[0].len;
    image_data += valid_data_offset;
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        app_log_error("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        goto err;
    }
    if (UPGRADE_SECT_APP == s_upgrade_info.para.section)
    {
        for(i = 0; i < mem_table.count; i++)
        {
            partition = &mem_table.tables[i];
            if(partition->used_size != 0)
                sub_file_index++;

            if((strcasecmp(mem_table.tables[i].name, DATA_SECTION_NAME) != 0))
            {
                if(partition->used_size != 0)
                {
                    ret = _usb_upgrade_partition_exec(p_sub_file[sub_file_index].start_addr - valid_data_offset + image_data, partition->used_size, partition);
                    if(ret == -1)
                        goto err;
                }
                else
                {
                    GxCore_Partition_Unprotect();
                    ret = GxCore_PartitionErase(partition);
                    if (ret != 1)
                    {
                        app_log_error("Partition Erase err\n");
                        goto err;
                    }
                }

            }
        }
    }
    else if(UPGRADE_SECT_DATA == s_upgrade_info.para.section)
    {
        flash_table = GxCore_PartitionFlashInit();
        partition = GxCore_PartitionGetByName(flash_table,DATA_SECTION_NAME);
        if(partition != NULL)
        {
            for(i=0; mem_table.count; i++)
            {
                partition = &mem_table.tables[i];
                if(partition->used_size != 0)
                    sub_file_index++;

                if((strcasecmp(mem_table.tables[i].name, DATA_SECTION_NAME) == 0))
                {
                    s_upgrade_info.total_size = partition->used_size;
                    ret = _usb_upgrade_partition_exec(p_sub_file[sub_file_index].start_addr - valid_data_offset + image_data, partition->used_size, partition);
                    if(ret == -1)
                        goto err;
                    break;
                }
            }
        }
    }
    else
    {
        s_upgrade_info.total_size = s_flash_size;
        for(i = 0; i < mem_table.count; i++)
        {
            partition = &mem_table.tables[i];
            if(partition->used_size != 0)
                sub_file_index++;

            if(partition->used_size != 0)
            {
                ret = _usb_upgrade_partition_exec(p_sub_file[sub_file_index].start_addr - valid_data_offset + image_data, partition->used_size, partition);
                if(ret == -1)
                    goto err;
            }
            else
            {
                GxCore_Partition_Unprotect();
                ret = GxCore_PartitionErase(partition);
                if (ret != 1)
                {
                    app_log_error("Partition Erase err\n");
                    goto err;
                }
            }
        }
    }
    s_upgrade_info.state = UPGRADE_SUCCESS;
    app_log_min("Upgrade Success \n");
    return;
err:
    s_upgrade_info.state = UPGRADE_STOPPED;
    if(hw_all_data.usr_p != NULL)
    {
        GxCore_HwFree((void*)&hw_all_data.usr_p);
        memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    }
    app_log_error("Upgrade Fail \n");
    return;
}



int app_usb_nand_upgrade(UpgradePara *upgrade_para)
{
    int thread_id = 0;
    memcpy(&(s_upgrade_info.para),upgrade_para,sizeof(UpgradePara));
    GxCore_ThreadCreate("nand_update_data", &thread_id, _usb_upgrade_nand_exec, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
    return 0;
}

static status_t _upgrade_usb_upgrade_exec(UpgradePara *upgrade_para)
{
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    int ret = 0;
    if(upgrade_para == NULL)
        return GXCORE_ERROR;
    memset(&s_upgrade_info,0,sizeof(UsbUpgradeInfo));
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_unset_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif
#if (OTA_SUPPORT > 0)
    extern void app_ota_backend_polling_stop(void);
    app_ota_backend_polling_stop();
#endif
#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#if DVB_CLOUD_SUPPORT
    gx_dvbonline_pause();
#endif
#endif
    app_upgrade_lowpower_load_firmware();
    app_copy_font_to_ram();
    GUI_CreateDialog(WND_UPGRADE_PROCESS);
    GUI_SetInterface("flush",NULL);
    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
    {
        app_log_error("Get Partition Type err\n");
        return GXCORE_ERROR;
    }
    s_upgrade_info.flash_type = flash_type;
    if(_check_upgrade_file_valid(upgrade_para) == false)
        goto upgrade_error;
    if (flash_type == GXPARTITION_NORFLASH)
    {
        if (_check_sections(upgrade_para) != 0)
            goto upgrade_error;

        app_usb_nor_upgrade(upgrade_para);
    }
    else
    {
        app_usb_nand_upgrade(upgrade_para);
    }

    return GXCORE_SUCCESS;

upgrade_error:
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    app_font_ram_destroy();
    s_upgrade_info.state = UPGRADE_STOPPED;
    app_upgrade_lowpower_free_firmware();
    if (hw_all_data.usr_p != NULL)
    {
	    GxCore_HwFree(&hw_all_data);
	    memset(&hw_all_data, 0, sizeof(GxHwMallocObj));
    }
    return GXCORE_ERROR;
}

static int app_upgrade_exec_button_press_callback(unsigned short key)
{
    UpgradePara upgrade_para = {0};
    PopDlg  pop = {0};

    if(key == STBK_OK)
    {
        app_upgrade_para_get(&upgrade_para);
        if((upgrade_para.type == UPGRADE_TYPE_U2S) || (upgrade_para.type == UPGRADE_TYPE_DUMP)) //usb
        {
            if(check_usb_status() == false)
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.str = STR_ID_NO_DEVICE;
                pop.mode = POP_MODE_UNBLOCK;
                pop.pos.x = APP_POP_DLG_POS_X;
                pop.pos.y = APP_POP_DLG_POS_Y;
                popdlg_create(&pop);
            }
            else
            {
                if((upgrade_para.path == NULL)
                        || (GxCore_FileExists(upgrade_para.path) == GXCORE_FILE_UNEXIST))
                {
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_OK;
                    pop.str = STR_ID_ERR_PATH;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.pos.x = APP_POP_DLG_POS_X;
                    pop.pos.y = APP_POP_DLG_POS_Y;
                    popdlg_create(&pop);
                }
                else
                {
                    s_flash_size = app_get_flash_size();
                    s_flash_size = ((s_flash_size >(4* SIZE_UNIT_M)) && (s_flash_size%(4* SIZE_UNIT_M)==0))?s_flash_size:(4* SIZE_UNIT_M);
                    if(upgrade_para.type == UPGRADE_TYPE_DUMP) {
                        if (check_usb_write_permissions(upgrade_para.path)) {
                            _upgrade_usb_dump_exec(&upgrade_para);
                        } else {
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_OK;
                            pop.str = STR_ID_NOWRITE_PERMISSIONS;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.pos.x = APP_POP_DLG_POS_X;
                            pop.pos.y = APP_POP_DLG_POS_Y;
                            popdlg_create(&pop);
                        }
                    } else {
                        _upgrade_usb_upgrade_exec(&upgrade_para);
                    }
                }
            }
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static void app_upgrade_type_item_init(void)
{
    int ret = 0;
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
        return;;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemTitle = STR_ID_UPGRADE_TYPE;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemType = ITEM_CHOICE;
    if (flash_type == GXPARTITION_NORFLASH)
        s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade,Dump]";
    else
        s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade]";
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel = UPGRADE_TYPE_U2S;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemCallback.cmbCallback.CmbChange= app_upgrade_type_change_callback;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_sect_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_SECT].itemTitle = STR_ID_SECTION;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemType = ITEM_CHOICE;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.content= "[All,App,User]";
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = UPGRADE_SECT_ALL;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemCallback.cmbCallback.CmbChange= app_upgrade_section_change_callback;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_path_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_PATH].itemTitle = STR_ID_FILE_PATH;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemType = ITEM_PUSH;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemCallback.btnCallback.BtnPress= app_upgrade_path_button_press_callback;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_exec_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemTitle = STR_ID_START;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemType = ITEM_PUSH;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemCallback.btnCallback.BtnPress= app_upgrade_exec_button_press_callback;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemStatus = ITEM_NORMAL;
}

void app_upgrade_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif

	app_fuse_spec_epg_stop_work();

	s_file_path =NULL;
	s_file_path_bak = NULL;

	memset(&s_upgrade_opt, 0 ,sizeof(s_upgrade_opt));

	s_upgrade_opt.menuTitle = STR_ID_FIRMWARE_UPGRADE;
	s_upgrade_opt.titleImage = "s_title_left_utility";
	s_upgrade_opt.itemNum = ITEM_UPGRADE_TOTAL;
	s_upgrade_opt.timeDisplay = TIP_HIDE;
	s_upgrade_opt.item = s_upgrade_item;
	app_upgrade_type_item_init();

	app_upgrade_sect_item_init();
	app_upgrade_path_item_init();
	app_upgrade_exec_item_init();

	s_upgrade_opt.exit = app_upgrade_exit_callback;

	app_system_set_create(&s_upgrade_opt);
}

void app_t2_usb_upgrade_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif

	s_file_path = NULL;
	s_file_path_bak = NULL;

	memset(&s_upgrade_opt, 0 ,sizeof(s_upgrade_opt));

	s_upgrade_opt.menuTitle = STR_ID_FIRMWARE_UPGRADE;
	s_upgrade_opt.titleImage = "s_title_left_utility";
	s_upgrade_opt.itemNum = ITEM_UPGRADE_TOTAL;
	s_upgrade_opt.timeDisplay = TIP_HIDE;
	s_upgrade_opt.topTipImgDisplay = TIP_HIDE;
	s_upgrade_opt.bottmTipDisplay = TIP_HIDE;
	s_upgrade_opt.item = s_upgrade_item;
	app_upgrade_type_item_init();

	app_upgrade_sect_item_init();
	app_upgrade_path_item_init();
	app_upgrade_exec_item_init();

	s_upgrade_opt.exit = app_upgrade_exit_callback;

	app_system_set_create(&s_upgrade_opt);
}

UpgradeTypeSel app_get_upgrade_type(void)
{
	return s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;
}

static void _progress_show_error(void)
{
    int rate = 100;

    remove_timer(sp_UpgradeTimeOut);
	sp_UpgradeTimeOut = NULL;

    GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);

    GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_ERR_OCCUR);
    GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);

    GUI_SetProperty("img_upgrade_process_tip", "state", "show");
    GUI_SetProperty("btn_upgrade_process_ok", "state", "show");
}

static int _timer_upgrade_timeout(void *userdata)
{
    int rate = 0;
    if(s_upgrade_info.state == UPGRADE_START)
    {
        if(s_upgrade_info.total_size > 0)
            rate = s_upgrade_info.cur_size*100/s_upgrade_info.total_size;
        GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);
    }
    else if(s_upgrade_info.state == UPGRADE_STOPPED)
    {
        remove_timer(sp_UpgradeTimeOut);
        sp_UpgradeTimeOut = NULL;
        _progress_show_error();
        GUI_SetProperty("img_upgrade_process_tip", "state", "show");
        GUI_SetProperty("btn_upgrade_process_ok", "state", "show");
    }
    else if(s_upgrade_info.state == UPGRADE_SUCCESS)
    {
        remove_timer(sp_UpgradeTimeOut);
        sp_UpgradeTimeOut = NULL;
        rate = 100;
        GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);
        if(s_upgrade_info.para.type == UPGRADE_TYPE_U2S)
        {
            GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_UPGRADE_SUCCESS);
            GUI_SetProperty("txt_upgrade_process_info2", "string", STR_ID_REBOOT_NOW);
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(2000);
            app_upgrade_reboot();
        }
        else
        {
            GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_DUMP_SUCCESS);
            GUI_SetProperty("txt_upgrade_process_info2", "string", " ");
            GUI_SetInterface("flush",NULL);
        }
    }

	return 0;
}

SIGNAL_HANDLER int app_upgrade_progcess_create(GuiWidget * widget, void *usrdata)
{
    int rate = 0;
    time_t timeout = 500;
    GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);
    UpgradeTypeSel type = app_get_upgrade_type();
    if(type == UPGRADE_TYPE_DUMP)
    {
        GUI_SetProperty("text_upgrade_process_tip", "string", STR_ID_DUMP);
    }
    else
    {
        GUI_SetProperty("text_upgrade_process_tip", "string", STR_ID_UPGRADE);
    }

    GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_WAITING);
    GUI_SetProperty("txt_upgrade_process_info2", "string", STR_ID_DONT_CUT_PWER);

    s_upgrade_info.state = UPGRADE_START;

    if (reset_timer(sp_UpgradeTimeOut) != 0)
    {
        sp_UpgradeTimeOut = create_timer(_timer_upgrade_timeout, timeout, NULL, TIMER_REPEAT);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_upgrade_progcess_destroy(GuiWidget *widget, void *usrdata)
{
    remove_timer(sp_UpgradeTimeOut);
    sp_UpgradeTimeOut = NULL;

    if(GxCore_FileExists(FONT_RAM_PATH) == GXCORE_FILE_EXIST)
    {
        GxCore_FileDelete(FONT_RAM_PATH);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_upgrade_progcess_keypress(GuiWidget* widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    UpgradeTypeSel update_type;

    update_type = app_get_upgrade_type();

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_OK:
                    if(s_upgrade_info.state == UPGRADE_STOPPED)
                    {
                        if(UPGRADE_TYPE_U2S == update_type)
                        {
                            app_font_ram_destroy();
                            app_upgrade_lowpower_free_firmware();
                        }
                        if(update_type == UPGRADE_TYPE_U2S || update_type == UPGRADE_TYPE_DUMP)
                        {
                            GUI_EndDialog(WND_UPGRADE_PROCESS);
                            app_system_set_focus_item(ITEM_UPGRADE_EXEC);
                        }
                    }
                    else if(s_upgrade_info.state == UPGRADE_SUCCESS)
                    {
                        if(update_type == UPGRADE_TYPE_DUMP)
                        {
                            GUI_EndDialog(WND_UPGRADE_PROCESS);
                            app_system_set_focus_item(ITEM_UPGRADE_EXEC);
                            app_upgrade_clear_path();
                        }
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}
