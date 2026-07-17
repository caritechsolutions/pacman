#ifdef ECOS_OS
#include "usb/kernel.h"
#include "gxos/gxcore_os_core.h"
#include "common/gxlog.h"
#endif
#include "config.h"

extern void boot_downloader(int argc, char **argv);

#ifdef ECOS_OS
void startup_thread(void *arg)
{
	boot_downloader(0, NULL);
}
#endif

int GxCore_Startup(int argc, char **argv)
{
#ifdef NO_OS
	boot_downloader(argc,argv);
#else
	static int thread = 0;
#ifdef ECOS_OS
extern int mount(const char *devname,
		const char *dir,
		const char *fsname,
		unsigned long mountflags,
		const void *data);
	if (mount("", "/", "ramfs", 0, NULL) != 0)
		gxlogd("mount ramfs failed\n");
#ifdef CONFIG_ENABLE_USB_UPGRADE
	/* The U disk will be automatically mounted to the /mnt/usb01 directory,
	 * and the /mnt directory must be created before mounting */
	if (mkdir("/mnt", 0777) != 0)
		printf("mkdir /mnt failed\n");
#endif
#endif
	GxCore_ThreadCreate("init", &thread, startup_thread, NULL,100 * 1024,GXOS_DEFAULT_PRIORITY);
	GxCore_Loop();
#endif
	return 0;
}
