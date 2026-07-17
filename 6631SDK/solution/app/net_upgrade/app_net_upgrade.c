/*************************************************************************
	> File Name: net_upgrade.c
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 15时45分01秒
 ************************************************************************/

#include "app.h"
#include "app_module.h"
#if NET_UPGRADE_SUPPORT
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "module/update/gxupdate_partition_file.h"
#include "module/update/gxupdate_partition_flash.h"
#include "module/update/gxupdate_protocol_usb.h"
#include "module/update/gxupdate_protocol_ts.h"
#include "module/update/gxupdate_protocol_serial.h"
#include "app_netapps_service.h"
#include "app_tools.h"
#include <common/gx_hw_malloc.h>
#include "app_net_upgrade.h"
#include "app_net_upgrade_common.h"
#include "app_upgrade_xml_parse.h"
#include "app_windows.h"

#define CYGNUS_PUBLIC_ID    0x6705
#define CANOPUS_PUBLIC_ID   0x6631
#define SCORPIO_PUBLIC_ID   0x3113
#define TAURUS_PUBLIC_ID    0x6616
#define PEGASUS_PUBLIC_ID   0x6605
#define NET_UPG_FLASH_CONFIG WORK_PATH"theme/upgrade_info.xml"
#define SW_VERSION_LEN      64
#define GPIO_POWERCUT_LEN 32

//#define LOADER_MAGICNUM_TEMP 0X12345678
#define TABLE_PARTITION_NAME "TABLE"
#define MOUNT_PATH "/home/gx"
#define RECOVERY_VRESION   MOUNT_PATH"/recovery_partition"
#define RECOVERY_PARTITION_HEAD_LEN 4096
#define LOADER_MAGIC_NUM1 0x55AA55AA
#define LOADER_MAGIC_NUM2 0x55BB55BB
#define UPDATE_IMAGE_MAGIC_NUM1 0xEBD88320
#define UPDATE_IMAGE_MAGIC_NUM2 0x12477CDF

#define UPGRADE_DEBUG_SUPPROT
#ifdef UPGRADE_DEBUG_SUPPROT
#define UPGRADE_DBG(fmt, args...)  do {                         \
    printf(fmt, ##args);                                     \
} while(0)
#else
#define UPGRADE_DBG(...) do{}while(0)
#endif
static GxHwMallocObj s_upgrade_recv_part_data_buf = {0};
GxHwMallocObj s_upgrade_bin_data_buf = {0};
GxHwMallocObj s_upgrade_recv_os_data_buf = {0};
static uint32_t s_flash_size = 4 * SIZE_UNIT_M;
static int32_t recv_os_offset_addr = 0;
static uint32_t s_upgrade_data_size = 0;
static uint32_t s_upgrade_recv_os_size = 0;
static bool s_recovery_partition_upgrade = false;
static struct partition *flash_table = NULL;
NetUpgSettingInfo *s_upg_setting_info = NULL;
extern int app_upgrade_crc_check_buf(char *p_data, int size);

typedef enum {
    NET_DEVICE_ETH = 0,     ///< eth
    NET_DEVICE_USBETH,      ///< usb eth
    NET_DEVICE_WIFI,        ///< wifi
    NET_DEVICE_MAX
}NetDeviceType;

typedef struct
{
    NetDeviceType type;
    ubyte32 wifi_ap_mac;
    ubyte32 wifi_ap_psk;
}UpgNetInfo;

typedef struct __attribute__ ((packed)) _RecoveryParHead
{
    unsigned int magicnum1;
    unsigned int magicnum2;
    char url[NETUPG_MAX_URL_LEN];
    char sw_ver[SW_VERSION_LEN];
    char recovery_partition[FLASH_PARTITION_NAME_SIZE + 1];
    char sw_ver_partition[FLASH_PARTITION_NAME_SIZE + 1];
    UpgNetInfo net_info;
    unsigned int file_size;
    char gpio_powercut[GPIO_POWERCUT_LEN];
    unsigned int header_crc32;
}RecoveryParHead;

typedef struct {
    void *image_data;
    uint32_t image_size;
}RecoveryParImg;
static RecoveryParImg recovery_img;
static UpgNetInfo s_net_info = {0};
static char s_recovery_gpio_powercut[GPIO_POWERCUT_LEN];

static void _netupg_net_info_get(void)
{
    ubyte64 cur_dev = {0};
    GxMsgProperty_CurAp cur_ap;

    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    memset(&cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    if(0 == strcmp(cur_dev, NET_IF_DEV_WIFI0))
    {
        s_net_info.type = NET_DEVICE_WIFI;
        strcpy(cur_ap.wifi_dev, cur_dev);
        app_send_msg_exec(GXMSG_WIFI_GET_CUR_AP, &cur_ap);
        memcpy(s_net_info.wifi_ap_mac, cur_ap.ap_mac, sizeof(ubyte32));
        memcpy(s_net_info.wifi_ap_psk, cur_ap.ap_psk, sizeof(ubyte32));
    }
    else if(0 == strcmp(cur_dev,NET_IF_DEV_ETH0))
        s_net_info.type = NET_DEVICE_ETH;
    else if(0 == strcmp(cur_dev,NET_IF_DEV_ETH1))
        s_net_info.type = NET_DEVICE_USBETH;

    return;
}

static void app_write_data_to_recovery_partition(unsigned int recovery_os_size)
{
    RecoveryParHead *rec_par_header = NULL;
    struct partition_info* partition = NULL;
    uint32_t header_offset_addr = 0;
    int ret = 0;
    int ssize = 0;
    int write_real_size = 0;
    unsigned int flash_block_size = 0;

    UPGRADE_DBG("write data to recovery partition start!!!!!!!!!!\n");
    rec_par_header = (RecoveryParHead*)GxCore_Malloc(sizeof(RecoveryParHead));
    if(rec_par_header == NULL)
    {
        UPGRADE_DBG("upgrade write recovery partition header malloc error \n");
        goto err;
    }
    else
    {
        memset(rec_par_header, 0, sizeof(RecoveryParHead));
        rec_par_header->magicnum1 = UPDATE_IMAGE_MAGIC_NUM1;
        rec_par_header->magicnum2 = UPDATE_IMAGE_MAGIC_NUM2;
        strcpy(rec_par_header->url, s_upg_setting_info->url);
        strcpy(rec_par_header->sw_ver, s_upg_setting_info->sw_ver);
        strcpy(rec_par_header->recovery_partition, s_upg_setting_info->rec_par);
        if(s_upg_setting_info->sw_ver_par)
            strcpy(rec_par_header->sw_ver_partition, s_upg_setting_info->sw_ver_par);
        memcpy(&(rec_par_header->net_info), &s_net_info, sizeof(UpgNetInfo));
    }
    rec_par_header->file_size = atoi(s_upg_setting_info->file_size);
    strcpy(rec_par_header->gpio_powercut,s_recovery_gpio_powercut);
    rec_par_header->header_crc32 = GxCore_Crc32(0, (uint8_t*)((char*)rec_par_header+8),sizeof(RecoveryParHead)-8-4);
    UPGRADE_DBG("write data size = %d : %s\n",rec_par_header->file_size,__func__);
    partition = GxCore_PartitionGetByName(flash_table, s_upg_setting_info->rec_par);

    ret = GxCore_PartitionGetFlashBlockSize(&flash_block_size);
    if ( (ret != 0) || (flash_block_size <=0 ) )
    {
        UPGRADE_DBG("GetFlashBlockSize err %d \n",flash_block_size);
        goto err;
    }
    header_offset_addr = partition->total_size - RECOVERY_PARTITION_HEAD_LEN;

    GxCore_Partition_Unprotect();
    ret = GxCore_PartitionErase(partition);
    if (ret != 1)
    {
        UPGRADE_DBG("Partition Erase err\n");
        goto err;
    }
    ret = GxCore_PartitionWriteRaw(partition, rec_par_header, sizeof(RecoveryParHead), header_offset_addr);
    if (ret != 1)
    {
        UPGRADE_DBG("Partition write header err\n");
        goto err;
    }
    s_upg_process.cur_size += RECOVERY_PARTITION_HEAD_LEN;
    UPGRADE_DBG("Upgrade Process %d  \n", s_upg_process.cur_size);
    APP_FREE(rec_par_header);

    ssize = 0;
    write_real_size = 0;
    while (ssize < recovery_os_size)
    {
        write_real_size = ((recovery_os_size - ssize) >= flash_block_size ? flash_block_size : (recovery_os_size - ssize));
        ret = GxCore_PartitionWriteRaw(partition, (char *)(s_upgrade_recv_os_data_buf.usr_p) + ssize, write_real_size, recv_os_offset_addr + ssize);
        if (ret != 1)
        {
            UPGRADE_DBG("Partition write recovery os err\n");
            goto err;
        }
        ssize += write_real_size;
        s_upg_process.cur_size += write_real_size;
        UPGRADE_DBG("Upgrade Process %d  \n", s_upg_process.cur_size);
    }

    if(s_upgrade_recv_os_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_os_data_buf);
        memset(&s_upgrade_recv_os_data_buf, 0, sizeof(GxHwMallocObj));
    }
    return;
err:
    APP_FREE(rec_par_header);
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_recv_part_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_part_data_buf);
        memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_recv_os_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_os_data_buf);
        memset(&s_upgrade_recv_os_data_buf, 0, sizeof(GxHwMallocObj));
    }
    return;
}

static int _flash_update_partition_exec(unsigned char *buf, int size, struct partition_info *partition)
{
    int ret = 0;
    int ssize = 0;
    int write_real_size = 0;
    unsigned int flash_block_size = 0;
    uint32_t magic = 0;
    int update_loader = 0;

    if((buf == NULL) || (partition == NULL) || (size == 0))
    {
        UPGRADE_DBG("para err!!!\n");
        return -1;
    }

    ret = GxCore_PartitionGetFlashBlockSize(&flash_block_size);
    if ( (ret != 0) || (flash_block_size <=0 ) )
    {
        UPGRADE_DBG("GetFlashBlockSize err %d \n",flash_block_size);
        return -1;
    }

    memcpy(&magic, buf, 4);
    if((magic == LOADER_MAGIC_NUM1) || (magic == LOADER_MAGIC_NUM2))
    {
        update_loader = 1;
    }

    GxCore_Partition_Unprotect();
    ret = GxCore_PartitionErase(partition);
    if (ret != 1)
    {
        UPGRADE_DBG("Partition Erase err\n");
        return -1;
    }
    ssize = 0;
    write_real_size = 0;
    UPGRADE_DBG("Part :| %s |Write Flash Addr=0x%08x, From Dram=%p,size:%d \n",
        partition->name, partition->start_addr, buf, size);
    if(update_loader == 1)
    {
        ssize = 8*1024;
        ret = GxCore_PartitionWriteRaw(partition, (void *)(buf+ssize), size - ssize, ssize);
        if (ret != 1)
        {
            UPGRADE_DBG("Partition write err \n");
            return -1;
        }
        ret = GxCore_PartitionWriteRaw(partition, (void *)(buf), ssize, 0);
        if (ret != 1)
        {
            UPGRADE_DBG("Partition write err \n");
            return -1;
        }
        s_upg_process.cur_size += size;
        UPGRADE_DBG("Upgrade Process %d  \n", s_upg_process.cur_size);
    }
    else
    {
        while (ssize < size)
        {
            write_real_size = ((size - ssize) >= flash_block_size ? flash_block_size : (size - ssize));
            ret = GxCore_PartitionWriteRaw(partition, (void *)(buf+ssize), write_real_size, ssize);
            if (ret != 1)
            {
                UPGRADE_DBG("Partition write err \n");
                return -1;
            }
            ssize += write_real_size;
            s_upg_process.cur_size += write_real_size;
            UPGRADE_DBG("Upgrade Process %d  \n", s_upg_process.cur_size);
        }
    }
    UPGRADE_DBG("%s partition upgrade success \n", partition->name);
    return 0;
}

static int _netupg_check_bin_file(void)
{
    struct partition mem_table = {0};
    unsigned char *image_data = NULL;
    int i = 0;
    unsigned char update_tag = 0;
    int old_cnt = 0;
    uint32_t magic = 0;
    int need_update_loader = 0;
    uint32_t chipid = 0;
#ifdef LINUX_OS
    int update_all_bin = 0;
#endif

    image_data = s_upgrade_bin_data_buf.usr_p;

    /*利用patition结构格式化info信息*/
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        return -1;
    }
    s_upgrade_data_size = 0;
    for(i = 0; i < mem_table.count; i++)
        s_upgrade_data_size += mem_table.tables[i].total_size;
    if(atoi(s_upg_setting_info->file_size) != s_upgrade_data_size)
    {
        UPGRADE_PRINTF("\nfile size error , file size = %s , bin size = %d\n",s_upg_setting_info->file_size,s_upgrade_data_size);
        return -1;
    }
    s_upgrade_data_size = 0;
    if(strcasecmp(mem_table.tables[0].name, TABLE_PARTITION_NAME) != 0)
    {
        need_update_loader = 1;
#ifdef LINUX_OS
        update_all_bin = 1;
#endif
        for(i = 0; i < mem_table.count; i++)
            s_upgrade_data_size += mem_table.tables[i].total_size;
        if(s_flash_size != s_upgrade_data_size)
        {
            UPGRADE_PRINTF("\nsize error, flash_size:%d, upgrade_size:%d...%s,%d\n", s_flash_size, s_upgrade_data_size, __FILE__, __LINE__);
            return -1;
        }
    }
    else
    {
        update_tag = mem_table.tables[0].update_tags;
        if (update_tag == 1)
        {
            need_update_loader = 1;
#ifdef LINUX_OS
            update_all_bin = 1;
#endif
            s_upgrade_data_size = mem_table.tables[1].total_size;
            if(s_flash_size != s_upgrade_data_size)
            {
                UPGRADE_PRINTF("\nsize error, flash_size:%d, upgrade_size:%d...%s,%d\n", s_flash_size, s_upgrade_data_size, __FILE__, __LINE__);
                return -1;
            }
            image_data += mem_table.tables[0].total_size;
            memset(&mem_table, 0x00, sizeof(struct partition));
            if(GxCore_MEM_Partition_Init((char*)(image_data), &mem_table) != 0)
            {
                UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
                return -1;
            }
            else if(1 != app_upgrade_crc_check_buf((char*)(image_data), s_upgrade_data_size))
            {
                UPGRADE_PRINTF("\nCRC Error...%s,%d\n", __FILE__, __LINE__);
                return -1;
            }
        }
        else
        {
            for(i = 0; i< mem_table.count; i++)
            {
                if(mem_table.tables[i].update_tags == 2)
                {
                    memcpy(&magic, mem_table.tables[i].start_addr + image_data, 4);
                    if((magic == LOADER_MAGIC_NUM1) || (magic == LOADER_MAGIC_NUM2))
                    {
                        need_update_loader = 1;
                        break;
                    }
                }
            }
            for(i = 0; i< mem_table.count; i++)
            {
                if((strcmp(mem_table.tables[i].name, TABLE_PARTITION_NAME) != 0) && (mem_table.tables[i].update_tags == 2))
                {
                    for(old_cnt=0; old_cnt<flash_table->count; old_cnt++)
                    {
                        if(strcasecmp(mem_table.tables[i].name, flash_table->tables[old_cnt].name) == 0)
                        {
                            if(mem_table.tables[i].used_size > flash_table->tables[old_cnt].partition_size)
                            {
                                UPGRADE_PRINTF("\n%s partition size is large ...%s,%d\n", mem_table.tables[i].name, __FILE__, __LINE__);
                                return -1;
                            }
                            s_upgrade_data_size += mem_table.tables[i].used_size;
                            break;
                        }
                    }
                    if(old_cnt == flash_table->count)
                    {
                        UPGRADE_PRINTF("\nnot find update partition:%s...%s,%d\n", mem_table.tables[i].name, __FILE__, __LINE__);
                        return -1;
                    }
                }
            }
        }
    }
    if(s_upg_setting_info->rec_url) //安全升级
    {
        if(need_update_loader == 1)
        {
            chipid = app_chip_id_get();
            if((chipid != CANOPUS_PUBLIC_ID) && (chipid != SCORPIO_PUBLIC_ID) && (chipid != CYGNUS_PUBLIC_ID) && (chipid != TAURUS_PUBLIC_ID) && (chipid != PEGASUS_PUBLIC_ID))
            {
                UPGRADE_PRINTF("\nchipid:0x%d not support safe upgrade,...%s,%d\n", chipid, __FILE__, __LINE__);
                return -2;
            }
        }
    }
#ifdef LINUX_OS
    else
    {
        if (update_all_bin == 1)
        {
            if(mem_table.count != flash_table->count)
            {
                UPGRADE_PRINTF("\nmem_table.count = %d, flash_table->count = %d\n", mem_table.count,flash_table->count);
                return -1;
            }
            for(i = 0; i < mem_table.count; i++)
            {
                if((mem_table.tables[i].total_size != flash_table->tables[i].total_size)
                        || (strcasecmp(mem_table.tables[i].name, flash_table->tables[i].name) != 0))
                {
                    UPGRADE_PRINTF("\nmem_table name = %s, size = %d, flash name = %s,size = %d\n",mem_table.tables[i].name, mem_table.tables[i].total_size,flash_table->tables[i].name,flash_table->tables[i].total_size);
                    return -1;
                }
            }
        }
    }
#endif
    return 0;
}

static int _netupg_check_recovery_os_size(unsigned int size)
{
    struct partition_info* partition = NULL;

    partition = GxCore_PartitionGetByName(flash_table, s_upg_setting_info->rec_par);
    if((recv_os_offset_addr < 0) || (partition->total_size - recv_os_offset_addr < size + RECOVERY_PARTITION_HEAD_LEN))
    {
        UPGRADE_PRINTF("\nrecovery os size is large,...%s,%d\n", __FILE__, __LINE__);
        return -1;

    }
    return 0;
}

void _upg_update_write_sw_version(void)
{
    FILE *file = NULL;
    int size = 0;

    struct partition_info* partition = NULL;

    extern int mount(const char *devname,
        const char *dir,
        const char *fsname,
        unsigned long mountflags,
        const void *data);

    flash_table = GxCore_PartitionFlashInit();
    if(flash_table == NULL)
    {
        UPGRADE_DBG("Partition Flash Init  err\n");
        return;
    }
    partition = GxCore_PartitionGetByName(flash_table, s_upg_setting_info->sw_ver_par);
    if (partition)
    {
#ifdef ECOS_OS
        char *fs_type = NULL;
        static char dir_cur[32] = {0};
        fs_type = GxCore_PartitionGetType(partition);
        memset(dir_cur, 0x00, sizeof(dir_cur));
        sprintf(dir_cur, "/dev/flash/0/0x%x,0x%x", partition->start_addr, partition->total_size);
        if (mount(dir_cur, "/home/gx", fs_type, 0, NULL) != 0)
        {
            UPGRADE_DBG("mount %s failed!\n", s_upg_setting_info->sw_ver_par);
            return;
        }
#else
#define MTD_DEVICE_NAME_SIZE 20
        char device_name[MTD_DEVICE_NAME_SIZE] = {0};
        memset(device_name, 0x00, sizeof(device_name));
        sprintf(device_name, "/dev/mtdblock%d", partition->mtd_id);
        if (mount(device_name, "/home/gx", GxCore_PartitionGetType(partition), 0, NULL) != 0) {
            UPGRADE_DBG("mount %s %s failed!\n", device_name,s_upg_setting_info->sw_ver_par);
            return;
        }
        UPGRADE_DBG("mount %s %s type = %s success\n", device_name,s_upg_setting_info->sw_ver_par,GxCore_PartitionGetType(partition));
#endif
        file = fopen(RECOVERY_VRESION, "w");
        if (file == NULL)
        {
            UPGRADE_DBG("write sw ver error!\n");
            return;
        }
        size = strlen(s_upg_setting_info->sw_ver);
        if(size != fwrite(s_upg_setting_info->sw_ver, 1, size, file))
            UPGRADE_DBG("write sw ver failed!\n");
        else
            UPGRADE_DBG("write sw ver %s success!\n", s_upg_setting_info->sw_ver);
        fclose(file);
    }
    return;
}

static void _flash_update_data(void *arg)
{
    int i = 0;
    int old_cnt = 0;
    int find_part = 0;
    struct partition mem_table = {0};
    unsigned char *image_data = NULL;
    struct partition_info* partition = NULL;
    bool upgrade_all_bin = false;
    unsigned char update_tag = 0;
    int ret = 0;
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    int size = 0;

    GxCore_ThreadDetach();

    UPGRADE_DBG("not safe upgrade start!!!!!!!!!\n");
    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
    {
        UPGRADE_DBG("Get Partition Type err\n");
        goto err;
    }
    image_data = s_upgrade_bin_data_buf.usr_p;
    /*利用patition结构格式化info信息*/
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        goto err;
    }

    if(strcasecmp(mem_table.tables[0].name, TABLE_PARTITION_NAME) != 0)
        upgrade_all_bin = true;
    else
        update_tag = mem_table.tables[0].update_tags;

    /*table 第一个字段标志为１代表强行升级整个bin，在这种情况只有两个分区，table + update*/
    if (update_tag == 1)
    {
        upgrade_all_bin = true;
        image_data += mem_table.tables[0].total_size;
        memset(&mem_table, 0x00, sizeof(struct partition));
        if(GxCore_MEM_Partition_Init((char*)(image_data),&mem_table) != 0)
        {
            UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
            goto err;
        }
    }
    s_upg_process.total_size = s_upgrade_data_size;
    for(i = 0; i < mem_table.count; i++)
    {
        if (upgrade_all_bin == true)
        {
            partition = &mem_table.tables[i];
            if (flash_type == GXPARTITION_NORFLASH)
                size = partition->total_size;
            else
                size = partition->used_size;
            ret = _flash_update_partition_exec(image_data + mem_table.tables[i].start_addr, size, partition);
            if(ret == -1)
                goto err;
        }
        else
        {
            i = (i == 0) ? 1 : i; //skip first partition "TABLE"
            update_tag = mem_table.tables[i].update_tags;
            switch(update_tag)
            {
                case 2:/* update */
                    break;
                default:
                    continue;
            }
            //对比收到数据的name和本地table里面的
            for(old_cnt=0; old_cnt<flash_table->count; old_cnt++)
            {
                if((strcasecmp(mem_table.tables[i].name, TABLE_PARTITION_NAME) != 0)
                    && (strcasecmp(mem_table.tables[i].name, flash_table->tables[old_cnt].name) == 0))
                {
                    find_part = 1;
                    break;
                }
            }
            if(find_part == 1)
            {
                partition = &flash_table->tables[old_cnt];
                size = mem_table.tables[i].used_size;
                ret = _flash_update_partition_exec(image_data + mem_table.tables[i].start_addr, size, partition);
                if(ret == -1)
                    goto err;
            }
        }
    }
//    _upg_update_write_sw_version();
    s_upg_process.cur_size = s_upg_process.total_size;
    s_upg_process.finish = 1;
    UPGRADE_DBG("Upgrade Success \n");
    return;
err:
    s_upg_process.finish = 2;
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    UPGRADE_DBG("Upgrade Fail \n");
    return;
}

static void _flash_update_partition_skip(void* arg)
{
    int i = 0;
    struct partition mem_table = {0};
    struct partition_info* partition = NULL;
    unsigned char *image_data = NULL;
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    int size = 0;
    int ret = 0;
    int old_cnt = 0;
    int find_part = 0;
    bool upgrade_all_bin = false;
    unsigned char update_tag = 0;

    GxCore_ThreadDetach();

    s_upg_process.total_size = s_upgrade_recv_os_size + RECOVERY_PARTITION_HEAD_LEN + s_upgrade_data_size;
    UPGRADE_DBG("safe upgrade start, total_size:%d\n", s_upg_process.total_size);
    app_write_data_to_recovery_partition(s_upgrade_recv_os_size);
    UPGRADE_DBG("write data to recovery partition finish!!!!!!!!!\n");

    image_data = s_upgrade_bin_data_buf.usr_p;
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        s_upg_process.finish = 2;
        goto err;
    }
    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
    {
        UPGRADE_PRINTF("Get Partition Type err\n");
        s_upg_process.finish = 2;
        goto err;
    }

    if(strcasecmp(mem_table.tables[0].name, TABLE_PARTITION_NAME) != 0)
        upgrade_all_bin = true;
    else
        update_tag = mem_table.tables[0].update_tags;

    /*table 第一个字段标志为１代表强行升级整个bin，在这种情况只有两个分区，table + update*/
    if (update_tag == 1)
    {
        upgrade_all_bin = true;
        image_data += mem_table.tables[0].total_size;
        memset(&mem_table, 0x00, sizeof(struct partition));
        if(GxCore_MEM_Partition_Init((char*)(image_data),&mem_table) != 0)
        {
            UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
            s_upg_process.finish = 2;
            goto err;
        }
    }

    for(i = 0; i < mem_table.count; i++)
    {
        if (upgrade_all_bin == true)
        {
#ifdef ECOS_OS
            if(strcmp(mem_table.tables[i].name, s_upg_setting_info->rec_par) != 0)
            {
                partition = &mem_table.tables[i];
                if (flash_type == GXPARTITION_NORFLASH)
                    size = partition->total_size;
                else
                    size = partition->used_size;
                ret = _flash_update_partition_exec(image_data + mem_table.tables[i].start_addr, size, partition);
                if(ret == -1)
                {
                    s_upg_process.finish = 2;
                    goto err;
                }
            }
#else
            s_upg_process.finish = 3;
            goto err;
#endif
        }
        else
        {
            i = (i == 0) ? 1 : i; //skip first partition "TABLE"
            update_tag = mem_table.tables[i].update_tags;
            switch(update_tag)
            {
                case 2:/* update */
                    break;
                default:
                    continue;
            }
            if(strcmp(mem_table.tables[i].name, s_upg_setting_info->rec_par) == 0)
                break;
            //对比收到数据的name和本地table里面的
            for(old_cnt=0; old_cnt<flash_table->count; old_cnt++)
            {
                if((strcasecmp(mem_table.tables[i].name, TABLE_PARTITION_NAME) != 0)
                    && (strcasecmp(mem_table.tables[i].name, flash_table->tables[old_cnt].name) == 0))
                {
                    find_part = 1;
                    break;
                }
            }
            if(find_part == 1)
            {
                partition = &flash_table->tables[old_cnt];
                size = mem_table.tables[i].used_size;
                ret = _flash_update_partition_exec(image_data + mem_table.tables[i].start_addr, size, partition);
                if(ret == -1)
                {
                    s_upg_process.finish = 2;
                    goto err;
                }
            }
        }
    }
    s_upg_process.cur_size = s_upg_process.total_size;
    s_upg_process.finish =1;
    UPGRADE_DBG("Upgrade Success \n");
    return;
err:
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_recv_part_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_part_data_buf);
        memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upg_process.finish == 2)
        UPGRADE_DBG("Upgrade Fail \n");
    else if(s_upg_process.finish == 3)
        UPGRADE_DBG("enter to recovery os \n");
    return;
}

static int app_safe_upgrade(void)
{
    int i = 0;
    struct partition mem_table = {0};
    unsigned char update_tag = 0;
    struct partition_info* partition = NULL;
    unsigned char *image_data = NULL;
    int partition_size = 0;
    int mem_start_addr = 0;
    int flash_start_addr = 0;
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    int ret = 0;

    image_data = s_upgrade_bin_data_buf.usr_p;
    if(GxCore_MEM_Partition_Init((char*)image_data, &mem_table) != 0)
    {
        UPGRADE_PRINTF("\nGxCore_MEM_Partition_Init Error...%s,%d\n", __FILE__, __LINE__);
        goto err;
    }

    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
    {
        UPGRADE_PRINTF("get partition type err\n");
        goto err;
    }
    s_recovery_partition_upgrade = false;
    recovery_img.image_data = NULL;
    recovery_img.image_size = 0;
    if(strcasecmp(mem_table.tables[0].name, TABLE_PARTITION_NAME) != 0)
    {
        for(i = 0; i < mem_table.count; i++)
        {
            if(0 == strcmp(mem_table.tables[i].name, s_upg_setting_info->rec_par))
            {
                partition_size = mem_table.tables[i].total_size;
                mem_start_addr = mem_table.tables[i].start_addr;
                if (flash_type == GXPARTITION_NORFLASH)
                    recovery_img.image_size = mem_table.tables[i].total_size;
                else
                    recovery_img.image_size = mem_table.tables[i].used_size;
                recovery_img.image_data = mem_table.tables[i].start_addr + image_data;
                s_recovery_partition_upgrade = true;
                break;
            }
        }
    }
    else
    {
        update_tag = mem_table.tables[0].update_tags;
        if(update_tag == 1)
        {
            image_data += mem_table.tables[0].total_size;
            if(GxCore_MEM_Partition_Init((char*)(image_data),&mem_table) == 0)
            {
                for(i = 0; i < mem_table.count; i++)
                {
                    if(0 == strcmp(mem_table.tables[i].name, s_upg_setting_info->rec_par))
                    {
                        partition_size = mem_table.tables[i].total_size;
                        mem_start_addr = mem_table.tables[i].start_addr;
                        if (flash_type == GXPARTITION_NORFLASH)
                            recovery_img.image_size = mem_table.tables[i].total_size;
                        else
                            recovery_img.image_size = mem_table.tables[i].used_size;
                        recovery_img.image_data = mem_table.tables[i].start_addr + image_data;
                        s_recovery_partition_upgrade = true;
                        break;
                    }
                }
            }
        }
        else
        {
            for(i = 0; i< mem_table.count; i++)
            {
                if((mem_table.tables[i].update_tags == 2) && (0 == strcmp(mem_table.tables[i].name, s_upg_setting_info->rec_par)))
                {
                    if (flash_type == GXPARTITION_NORFLASH)
                        recovery_img.image_size = mem_table.tables[i].total_size;
                    else
                        recovery_img.image_size = mem_table.tables[i].used_size;
                    recovery_img.image_data = mem_table.tables[i].start_addr + image_data;
                    s_recovery_partition_upgrade = true;
                    break;
                }
            }
        }
    }

    partition = GxCore_PartitionGetByName(flash_table, s_upg_setting_info->rec_par);
    flash_start_addr = partition->start_addr;
    if(!s_recovery_partition_upgrade)
    {
        memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
        s_upgrade_recv_part_data_buf.size = partition->total_size + 4;/*need add 4bytes*/
        GxCore_HwMalloc(&s_upgrade_recv_part_data_buf, MALLOC_WITH_CACHE);
        if(s_upgrade_recv_part_data_buf.usr_p == NULL)
        {
            UPGRADE_DBG("---------malloc s_upgrade_recv_part_data_buf error.\n");
            goto err;
        }
        GxCore_PartitionRead(partition, s_upgrade_recv_part_data_buf.usr_p, partition->total_size, 0);
        recv_os_offset_addr = 0;
    }
    else
    {
        if(mem_start_addr <= flash_start_addr)
            recv_os_offset_addr = 0;
        else if((mem_start_addr > flash_start_addr) && (mem_start_addr < flash_start_addr + partition->total_size))
            recv_os_offset_addr = mem_start_addr - flash_start_addr;
        else
            goto err;
    }
    partition_size = partition->total_size - recv_os_offset_addr;
    memset(&s_upgrade_recv_os_data_buf, 0, sizeof(GxHwMallocObj));
    s_upgrade_recv_os_data_buf.size = partition_size + 4;/*need add 4bytes*/
    GxCore_HwMalloc(&s_upgrade_recv_os_data_buf, MALLOC_WITH_CACHE);
    if(s_upgrade_recv_os_data_buf.usr_p == NULL)
    {
        UPGRADE_PRINTF("GxCore_HwMalloc  err\n");
        goto err;
    }

    memset(&s_upg_process, 0, sizeof(NetUpgProcess));
    s_upg_process.mode = NET_UPG_DOWNLOAD_RECOVERY_OS;
    app_upgrade_file_get(s_upg_setting_info->rec_url);

    return 0;
err:
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_recv_part_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_part_data_buf);
        memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
    }
    return -1;
}

int  _net_update_flash_unmount(char *mount_path)
{
    int ret = 0;

    if ( mount_path == NULL )
        return -1;
    GxBus_PmLock();
    GxBus_PmDbaseClose();
    GxBus_PmUnlock();
    handle_t h_service = GxBus_ServiceFindByName("book service");
    GxBus_ServiceDestroy(h_service); //delete boot.db
#ifdef ECOS_OS
    extern int umount(const char *name);
        ret = umount(mount_path);
#else
    char cmd[100] = {0};

    sprintf(cmd, "umount -f %s", mount_path);

    UPGRADE_DBG("umount cmd = %s \n",cmd);
    ret = system(cmd);
    if(ret != 0)
    {
        UPGRADE_DBG("umout ret=%d failed, please check the following files and processes!\n",ret);
        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"lsof | grep -w %s",mount_path);
        ret = system(cmd);
        return -1;
    }

#endif
    UPGRADE_DBG("umout ret=%d, path=%s \n",ret, mount_path);

    return ret;
}

int app_upgrade(void)
{
    int thread_id = 0;
    extern void app_copy_font_to_ram(void);
    extern void app_font_ram_destroy(void);

    app_copy_font_to_ram();
#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#if DVB_CLOUD_SUPPORT
    gx_dvbonline_pause();
#endif
#endif
    memset(s_recovery_gpio_powercut, 0, GPIO_POWERCUT_LEN);
    if(GXCORE_SUCCESS != app_get_gpio_config(GPIO_POWERCUT,s_recovery_gpio_powercut,GPIO_POWERCUT_LEN))
    {
        app_log_error("app_upgrade_lowpower_load_firmware gpio failer \n");
        return -1;
    }
    memset(&s_upg_process, 0, sizeof(NetUpgProcess));
    if(_net_update_flash_unmount(MOUNT_PATH) == -1)
    {
        app_font_ram_destroy();
        UPGRADE_DBG("umout failed \n");
        return 0;
    }
    GUI_CreateDialog(WND_NETUPG_PROCESS);
    if(s_upg_setting_info->rec_url) //安全升级
    {
        s_upg_process.mode = NET_UPG_UPDATE_SKIP_RECOVERY_PART;
        GxCore_ThreadCreate("flash_update_partition", &thread_id, _flash_update_partition_skip, NULL, 32 * 1024, GXOS_DEFAULT_PRIORITY);
    }
    else
    {
        s_upg_process.mode = NET_UPG_UPDATE_NO_SAFE;
        GxCore_ThreadCreate("flash_update_data", &thread_id, _flash_update_data, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
    }
    return 0;
}

int upgrade_exec(void)
{
    int ret = 0;

    if(s_upg_setting_info->rec_url) //安全升级
    {
        ret = app_safe_upgrade();
    }
    else    //非安全升级
    {
        app_upgrade();
    }

    return ret;
}

NetUpgProcess upgrade_process_get(void)
{
    return s_upg_process;
}

static int app_upg_hex_str_ver_compare(const char *ver1, const char *ver2)
{
    char *endptr1 = NULL, *endptr2 = NULL;
    unsigned long num1 = 0, num2 = 0;

    while (*ver1 != '\0' && *ver2 != '\0') {
        num1 = strtoul(ver1, &endptr1, 16);
        num2 = strtoul(ver2, &endptr2, 16);

        if (num1 < num2) {
            return -1;
        } else if (num1 > num2) {
            return 1;
        }
        ver1 = endptr1 + 1;
        ver2 = endptr2 + 1;
    }
    return 0;
}

static int app_upg_model_sn_check(const char *sn, const char *range)
{
    char *token = NULL;
    char *rangeCopy = strdup(range);
    int inRange = -1;

    token = strtok(rangeCopy, ",");
    while (token != NULL) {
        if (strstr(token, "-")) {
            char *startToken = strtok(token, "-");
            char *endToken = strtok(NULL, "-");

            unsigned long start = strtoul(startToken, NULL, 16);
            unsigned long end = strtoul(endToken, NULL, 16);
            unsigned long snVal = strtoul(sn, NULL, 16);

            if (snVal >= start && snVal <= end) {
                inRange = 0;
                break;
            }
        } else {
            unsigned long rangeVal = strtoul(token, NULL, 16);
            unsigned long snVal = strtoul(sn, NULL, 16);

            if (rangeVal == snVal) {
                inRange = 0;
                break;
            }
        }

        token = strtok(NULL, ",");
    }

    free(rangeCopy);
    return inRange;
}

static int _upg_parse_config_xml(char *path, NetUpgSettingInfo **upgrade_info)
{
    FILE *file = NULL;
    char *buffer = NULL;
    long file_size = 0;
    int ret = -1;

    if((path == NULL) || (strlen(path) == 0) || (upgrade_info == NULL))
    {
        UPGRADE_PRINTF("input error\n");
        return ret;
    }

    if(GXCORE_FILE_EXIST == GxCore_FileExists(path))
    {
        file = fopen(path, "rb");
        if (file == NULL)
            return ret;

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0L, SEEK_SET);
        buffer = (char *)GxCore_Mallocz(file_size * sizeof(char));
        if (buffer == NULL)
        {
            fclose(file);
            return ret;
        }
        fread(buffer, 1, file_size, file);
        fclose(file);
        ret = upgrade_parse_data_xml(buffer, file_size, upgrade_info);
        APP_FREE(buffer);
    }
    return ret;
}

static void _add_config_info(char** info)
{
    int total_len = 0;
    char *msg_str = NULL;

    total_len = strlen(s_upg_setting_info->sw_ver) + strlen(s_upg_setting_info->url);
    if(s_upg_setting_info->sw_ver_match)
        total_len += strlen(s_upg_setting_info->sw_ver_match);
    if(s_upg_setting_info->tip_timeout)
        total_len += strlen(s_upg_setting_info->tip_timeout);
    if(s_upg_setting_info->log)
        total_len += strlen(s_upg_setting_info->log);
    total_len += 10;
    msg_str = GxCore_Mallocz(total_len);
    if (msg_str == NULL)
        return;
    strcat(msg_str, s_upg_setting_info->sw_ver);
    strcat(msg_str, NETUPG_CONFIG_INFO_DELEMITER);
    if(s_upg_setting_info->sw_ver_match)
        strcat(msg_str, s_upg_setting_info->sw_ver_match);
    else
        strcat(msg_str, NETUPG_CONFIG_INFO_INVALID_FILL);
    strcat(msg_str, NETUPG_CONFIG_INFO_DELEMITER);
    strcat(msg_str, s_upg_setting_info->url);
    strcat(msg_str, NETUPG_CONFIG_INFO_DELEMITER);
    if(s_upg_setting_info->tip_timeout)
        strcat(msg_str, s_upg_setting_info->tip_timeout);
    else
        strcat(msg_str, NETUPG_CONFIG_INFO_INVALID_FILL);
    strcat(msg_str, NETUPG_CONFIG_INFO_DELEMITER);
    if(s_upg_setting_info->log)
        strcat(msg_str, s_upg_setting_info->log);
    else
        strcat(msg_str, NETUPG_CONFIG_INFO_INVALID_FILL);

    *info = msg_str;
    return;
}

int config_file_parse(char **info)
{
    char temp[SW_VERSION_LEN] = {0};
    FILE *file = NULL;
    NetUpgSettingInfo *flash_bin_upgrade_info = NULL;
    int ret = 0;

    UPGRADE_DBG("get_upgrade setting info!\n");
    ret = _upg_parse_config_xml(UPG_SERVER_CONFIG_FILE, &s_upg_setting_info);
    if((ret != 0) || (s_upg_setting_info  == NULL))
    {
        *info = GxCore_Strdup(STR_ID_FILE_PARSE_ERROR);
        return -1;
    }
    else
    {
        if((s_upg_setting_info->sw_ver == NULL) || (s_upg_setting_info->hw_ver == NULL) || (s_upg_setting_info->manufacture_id == NULL)
            || (s_upg_setting_info->url == NULL) || (s_upg_setting_info->file_size == NULL) || ((s_upg_setting_info->sw_ver_match != NULL)
            &&(0 != strcmp(s_upg_setting_info->sw_ver_match, "0"))&&(0 != strcmp(s_upg_setting_info->sw_ver_match, "1"))))
        {
            *info = GxCore_Strdup(STR_ID_SETTING_FILE_ERROR);
            return -1;
        }
        if((s_upg_setting_info->rec_url != NULL) && (s_upg_setting_info->rec_par == NULL))
        {
            *info = GxCore_Strdup(STR_ID_RECOVERY_PARTITION_INVALID);
            return -1;
        }
        if ((s_upg_setting_info->rec_url != NULL) && (s_upg_setting_info->rec_par != NULL))
        {
            struct partition *ptable = NULL;
            struct partition_info* partition = NULL;
            ptable = GxCore_PartitionFlashInit();
            if(ptable != NULL)
            {
                partition = GxCore_PartitionGetByName(ptable, s_upg_setting_info->rec_par);
                if((partition == NULL) || (partition->mode != 1))
                {
                    *info = GxCore_Strdup(STR_ID_RECOVERY_PARTITION_INVALID);
                    return -1;
                }
            }
        }
        UPGRADE_DBG("get_upgrade flash info!\n");
        ret = _upg_parse_config_xml(NET_UPG_FLASH_CONFIG, &flash_bin_upgrade_info);
        if((ret != GXCORE_SUCCESS) || (flash_bin_upgrade_info  == NULL))
        {
            *info = GxCore_Strdup(STR_ID_STB_INFO_PARSE_ERROR);
            goto err;
        }
        else
        {
            memset(temp, 0, sizeof(SW_VERSION_LEN));
            strcpy(temp, flash_bin_upgrade_info->hw_ver);
            if(app_upg_hex_str_ver_compare(s_upg_setting_info->hw_ver, temp) !=0)
            {
                *info = GxCore_Strdup(STR_ID_HARDWARE_VERSION_MISMATCH);
                goto err;
            }
            memset(temp, 0, sizeof(SW_VERSION_LEN));
            strcpy(temp, flash_bin_upgrade_info->manufacture_id);
            if(app_upg_hex_str_ver_compare(s_upg_setting_info->manufacture_id, temp) !=0)
            {
                *info = GxCore_Strdup(STR_ID_MANUFACTURE_ID_MISMATCH);
                goto err;
            }
            if(s_upg_setting_info->model_sn != NULL)
            {
                memset(temp, 0, sizeof(SW_VERSION_LEN));
                strcpy(temp, flash_bin_upgrade_info->model_sn);
                if(app_upg_model_sn_check(temp, s_upg_setting_info->model_sn) != 0)
                {
                    *info = GxCore_Strdup(STR_ID_MODEL_SN_MISMATCH);
                    goto err;
                }
            }
            if((s_upg_setting_info->sw_ver_match == NULL) || (0 == strcmp(s_upg_setting_info->sw_ver_match, "1")))
            {
                memset(temp, 0, sizeof(SW_VERSION_LEN));
                file = fopen(RECOVERY_VRESION, "r");
                if (file == NULL)
                {
                    strcpy(temp, flash_bin_upgrade_info->sw_ver);
                    UPGRADE_DBG("recovery_version file not exist, flash sw version: %s!\n", flash_bin_upgrade_info->sw_ver);
                }
                else
                {
                    fgets(temp, SW_VERSION_LEN, file);
                    UPGRADE_DBG("recovery_version file sw version: %s!\n", temp);
                    fclose(file);
                }
                if(app_upg_hex_str_ver_compare(s_upg_setting_info->sw_ver, temp) <= 0)
                {
                    *info = GxCore_Strdup(STR_ID_FIREWARE_NEWEST);
                    goto err;
                }
            }
            gx_free_upgrade_info(flash_bin_upgrade_info);
            _add_config_info(info);
            return 0;
        }
    }
err:
    gx_free_upgrade_info(flash_bin_upgrade_info);
    return -1;
}

void netupg_download_callback(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

    if(s_upg_process.mode ==  NET_UPG_DOWNLOAD_CONFIG)
    {
        if(message == GXCURL_STATE_COMPLETE)
        {
            if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
            {
                s_upg_process.finish = 1;
                UPGRADE_DBG("Downloading setting file finish!\n");
            }
            else
            {
                s_upg_process.finish = 2;
                UPGRADE_DBG("Downloading setting file error!\n");
            }
        }
    }
    else if(s_upg_process.mode == NET_UPG_DOWNLOAD_BIN)
    {
        if(message == GXCURL_STATE_PROCESS)
        {
            s_upg_process.total_size = callback_data->process_data.content_length;
            s_upg_process.cur_size = callback_data->process_data.download_size;
            UPGRADE_DBG("Downloading bin file cur_sise:%d, total_size:%d\n", s_upg_process.cur_size, s_upg_process.total_size);
        }
        else if(message == GXCURL_STATE_COMPLETE)
        {
            if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
            {
                s_upg_process.finish = 1;
                UPGRADE_DBG("Downloading bin file finish!\n");
            }
            else
            {
                s_upg_process.finish = 2;
                UPGRADE_DBG("Downloading bin file error!\n");
                if(s_upgrade_bin_data_buf.usr_p != NULL)
                {
                    GxCore_HwFree(&s_upgrade_bin_data_buf);
                    memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
                }
            }
        }
    }
    else if(s_upg_process.mode ==  NET_UPG_DOWNLOAD_RECOVERY_OS)
    {
        if(message == GXCURL_STATE_PROCESS)
        {
            s_upg_process.total_size = callback_data->process_data.content_length;
            s_upg_process.cur_size = callback_data->process_data.download_size;
            UPGRADE_DBG("Downloading bin revovery os cur_sise:%d, total_size:%d\n", s_upg_process.cur_size, s_upg_process.total_size);
        }
        else if(message == GXCURL_STATE_COMPLETE)
        {
            if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
            {
                s_upg_process.finish = 1;
                _netupg_net_info_get();
                s_upgrade_recv_os_size = s_upg_process.total_size;
                UPGRADE_DBG("Downloading recovery os finish!\n");
            }
            else
            {
                s_upg_process.finish = 2;
                UPGRADE_DBG("Downloading recovery os error!\n");
                if(s_upgrade_bin_data_buf.usr_p != NULL)
                {
                    GxCore_HwFree(&s_upgrade_bin_data_buf);
                    memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
                }
                if(s_upgrade_recv_part_data_buf.usr_p != NULL)
                {
                    GxCore_HwFree(&s_upgrade_recv_part_data_buf);
                    memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
                }
                if(s_upgrade_recv_os_data_buf.usr_p != NULL)
                {
                    GxCore_HwFree(&s_upgrade_recv_os_data_buf);
                    memset(&s_upgrade_recv_os_data_buf, 0, sizeof(GxHwMallocObj));
                }
            }
        }
    }
}

int upgrade_file_verify(char** tip)
{
    int ret = 0;

    if(s_upg_process.mode == NET_UPG_DOWNLOAD_BIN)
    {
        if(1 == app_upgrade_crc_check_buf(s_upgrade_bin_data_buf.usr_p, s_upg_process.total_size))
        {
            ret = _netupg_check_bin_file();
            if(ret == 0)
                return 0;
            else if(ret == -2)
                *tip = GxCore_Strdup(STR_ID_NOT_SUPPORT_SAFE_UPGRADE);
            else
                *tip = GxCore_Strdup(STR_ID_FIRMWARE_FAILED);
        }
        else
        {
            *tip = GxCore_Strdup(STR_ID_FIRMWARE_FAILED);
            UPGRADE_PRINTF("bin file crc error!\n");
        }
    }
    else if(s_upg_process.mode == NET_UPG_DOWNLOAD_RECOVERY_OS)
    {
        if(1 == app_upgrade_crc_check_buf(s_upgrade_recv_os_data_buf.usr_p, s_upg_process.total_size))
        {
            if(0 == _netupg_check_recovery_os_size(s_upg_process.total_size))
                return 0;
            else
                *tip = GxCore_Strdup(STR_ID_RECOVERY_OS_FAILED);
        }
        else
        {
            *tip = GxCore_Strdup(STR_ID_RECOVERY_OS_FAILED);
            UPGRADE_PRINTF("Recovery OS file crc error!\n");
        }
    }
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_recv_os_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_os_data_buf);
        memset(&s_upgrade_recv_os_data_buf, 0, sizeof(GxHwMallocObj));
    }
    return -1;
}

static int _upg_update_recovery_partition(void)
{
    int ret = 0;
    struct partition_info* partition = NULL;

    flash_table = GxCore_PartitionFlashInit();
    if(flash_table == NULL)
    {
        UPGRADE_DBG("Partition Flash Init  err\n");
        return -1;
    }
    partition = GxCore_PartitionGetByName(flash_table, s_upg_setting_info->rec_par);
    if(partition == NULL)
    {
        UPGRADE_PRINTF("\nget partition failed...%s,%d\n", __FILE__, __LINE__);
        return -1;
    }

    if(s_recovery_partition_upgrade)
    {
        if(recovery_img.image_data == NULL)
        {
            UPGRADE_DBG("update image not contain recovery img\n");
            return -1;
        }

        s_upg_process.total_size += recovery_img.image_size;
        UPGRADE_DBG("upgrade recovery partition start!!!!!!!!!\n");
        ret = _flash_update_partition_exec(recovery_img.image_data, recovery_img.image_size, partition);
    }
    else
    {
        UPGRADE_DBG("restore recovery partition start!!!!!!!!!\n");
        s_upg_process.total_size += s_upgrade_recv_part_data_buf.size-4;
        ret = _flash_update_partition_exec(s_upgrade_recv_part_data_buf.usr_p, s_upgrade_recv_part_data_buf.size-4, partition);
    }
    return ret;
}

void app_upg_main_upgrade_finish_process(void)
{
    int ret = 0;

    memset(&s_upg_process, 0, sizeof(NetUpgProcess));
    s_upg_process.mode = NET_UPG_UPDATE_RECOVERY_PART;

    ret = _upg_update_recovery_partition();
    if(s_upgrade_recv_part_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_recv_part_data_buf);
        memset(&s_upgrade_recv_part_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(s_upgrade_bin_data_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_upgrade_bin_data_buf);
        memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));
    }
    if(ret == 0)
    {
        _upg_update_write_sw_version();
        s_upg_process.finish = 1;
        UPGRADE_DBG("recovery partition process finish \n");
    }
    else
    {
        s_upg_process.finish = 2;
        UPGRADE_DBG("recovery partition process error \n");
        return;
    }
}

static void _upg_software_download_stop(void)
{
    if(s_upg_process.handle)
    {
        gxapps_curl_async_abort(s_upg_process.handle);
    }
}

int app_upgrade_file_get_init(char* url)
{
    enum GXPARTITION_FLASH_TYPE flash_type = 0;
    int ret = 0;

    //获取原有partition表
    flash_table = GxCore_PartitionFlashInit();
    if(flash_table == NULL)
    {
        UPGRADE_DBG("Partition Flash Init  err\n");
        return -1;
    }
    ret = GxCore_PartitionGetFlashType(&flash_type);
    if(ret != 0 )
    {
        UPGRADE_DBG("get partition type err\n");
        return -1;
    }
    if(flash_type == GXPARTITION_NORFLASH)
    {
        s_flash_size = app_get_flash_size();
    }
    else
    {
        UPGRADE_DBG("---------not support nand\n");
        return -1;
    }
    s_flash_size = ((s_flash_size >(4* SIZE_UNIT_M)) && (s_flash_size%(4* SIZE_UNIT_M)==0))?s_flash_size:(4* SIZE_UNIT_M);
    memset(&s_upgrade_bin_data_buf, 0, sizeof(GxHwMallocObj));

    s_upgrade_bin_data_buf.size = (atoi(s_upg_setting_info->file_size) + 4);/*64K for ota bin table,4bytes for hw*/
    GxCore_HwMalloc(&s_upgrade_bin_data_buf, MALLOC_WITH_CACHE);
    if(s_upgrade_bin_data_buf.usr_p == NULL)
    {
        UPGRADE_DBG("---------malloc s_upgrade_bin_data_buf error.\n");
        return -1;
    }
    _upg_software_download_stop();
    memset(&s_upg_process, 0, sizeof(NetUpgProcess));
    s_upg_process.mode = NET_UPG_DOWNLOAD_BIN;
    s_upg_process.total_size = s_flash_size;

    app_upgrade_file_get(url);
    return 0;
}

static void _netupg_download_stop(void)
{
    if(s_upg_process.handle)
    {
        gxapps_curl_async_abort(s_upg_process.handle);
    }
    memset(&s_upg_process, 0, sizeof(NetUpgProcess));
    if(GXCORE_FILE_EXIST == GxCore_FileExists(UPG_SERVER_CONFIG_FILE))
    {
        GxCore_FileDelete(UPG_SERVER_CONFIG_FILE);
    }

}

int app_net_upgrade_exit_callback(void)
{
    _netupg_download_stop();
    APP_FREE(s_upg_setting_info);
    return 0;
}

#endif
