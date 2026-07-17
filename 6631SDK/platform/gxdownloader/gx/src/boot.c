#ifdef NO_OS
#include "gxloader.h"
#include "boot.h"
#include "io.h"
#endif
#include "gxtype.h"

#include "config.h"
#include "common.h"
#include "gxdlp_api.h"
#include "gui.h"
#ifdef CONFIG_ENABLE_PANEL
#include "panel.h"
#endif
#ifdef CONFIG_ENABLE_IRR
#include "keyboard.h"
#endif
#include "common/gx_crc32.h"
#include "av.h"
#include "upgrade.h"
#include "secure.h"

#ifdef ECOS_OS
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_cache.h>

#ifdef csky
void boot_close_module(void)
{
	cyg_interrupt_disable();
	/* flush D-cache, dcache clr & inv*/
	asm volatile(
			"movi r7, 0x32;"
			"sync;"
			"mtcr r7, cr17;"
			:
			:
			:"r7","memory");
	/* flush I-cache, icache clr & inv*/
	asm volatile(
			"movi r7, 0x31;"
			"sync;"
			"mtcr r7, cr17;"
			:
			:
			:"r7","memory");

}
#else
size_t strnlen(const char *s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

void boot_close_module(void)
{

}
#endif

extern int doboot_kernel(struct partition_info *partition);
#endif

void boot(void)
{
#ifdef NO_OS
	//TODO:: check crc boot
	struct partition_info *partition_kernel = NULL;
	partition_kernel = all_partition_get(PARTITION_KERNEL);
	doboot_kernel(partition_kernel);
#else
	struct partition_info *partition_kernel = NULL;
	struct partition *ptable = NULL;

	ptable = GxCore_PartitionFlashInit();
	partition_kernel = GxCore_PartitionGetByName(ptable, PARTITION_KERNEL);
	doboot_kernel(partition_kernel);
#endif
}

void boot_downloader(int argc, const char **argv)
{
#ifdef CONFIG_ENABLE_PANEL
	unsigned char value = GXLED_OTA;
#endif
	char force = -1;
	unsigned int chip_definition;
	GxDLP_Download_Type download_type = GXDLP_NONE_DOWNLOAD;
	GxDLP_Init(CONFIG_DLP_PATH, CONFIG_BACKUP_DLP_PATH);//默认支持双备份
	secure_init();

#ifdef CONFIG_ENABLE_LOGO
	chip_definition = avinit();

#ifdef CONFIG_ENABLE_GUI
	guiinit(chip_definition);
#elif defined(CONFIG_ENABLE_GDI)
	gdiinit(chip_definition);
#endif
#endif

#ifdef CONFIG_ENABLE_PANEL
	panel_register(&fd650_runde);
#endif

#ifdef CONFIG_ENABLE_IRR
	keyboard_register(key_gongban_nationalchip_new);
#endif

#ifdef CONFIG_ENABLE_PANEL
	force = panel_updatetype();
	if(force != -1)
		GxDLP_Set_UpdateType(force);
#endif
#ifdef CONFIG_ENABLE_UPGRADE
	GxDLP_Get_UpdateType(&download_type);
	if (download_type != GXDLP_NONE_DOWNLOAD
		&& download_type != GXDLP_OTA_FAILED) {
#ifndef CONFIG_ENABLE_LOGO
		chip_definition = avinit();

#ifdef CONFIG_ENABLE_GUI
		guiinit(chip_definition);
#elif defined(CONFIG_ENABLE_GDI)
		gdiinit(chip_definition);
#endif
#endif

#ifdef CONFIG_ENABLE_PANEL
		value = GXLED_OTA;
		panel_showstring(&value);
#endif
#if defined(CONFIG_ENABLE_GDI) || defined(CONFIG_ENABLE_GUI)
		showmenu(force);
#endif
		upgrade();
		check_partition_status(1);
#if defined(CONFIG_ENABLE_GDI) || defined(CONFIG_ENABLE_GUI)
		hidemenu();
#endif
	}
#endif

#ifdef CONFIG_ENABLE_PANEL
	value = GXLED_BOOT;
	panel_showstring(&value);
#endif
#ifdef CONFIG_ENABLE_LOGO
	showlogo("logo");
#endif
	GxDLP_Close();
	boot();
}

