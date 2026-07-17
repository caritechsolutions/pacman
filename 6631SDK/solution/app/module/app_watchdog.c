#include "app_module.h"

#ifdef LINUX_OS
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define WDT_DEV "/dev/watchdog"
static int s_watchdog_fd = -1;
#define WATCHDOG_TIMEOUT 6
#define WATCHDOG_REBOOT_TIME WATCHDOG_TIMEOUT*10

static event_list* watch_dog_timer = NULL;

static int app_watchdog_feed(void* usrdata)
{
    if(s_watchdog_fd == -1)
    {
        return -1;
    }

    ioctl(s_watchdog_fd, WDIOC_KEEPALIVE, NULL);

    return 0;
}

status_t app_watchdog_init(void)
{
    int timeout = WATCHDOG_REBOOT_TIME*1000;

    if(s_watchdog_fd == -1)
    {
        if((s_watchdog_fd = open(WDT_DEV, O_RDWR)) <= 0)
        {
            s_watchdog_fd = -1;
            app_log_error("[%s][%d] open watchdog err !!!\n", __func__, __LINE__);
            return GXCORE_ERROR;
        }
        //app_log_debug("[%s][%d] open watchdog %d success, %d sec!!!\n", __func__, __LINE__, s_watchdog_fd, timeout);
        ioctl(s_watchdog_fd, WDIOS_ENABLECARD, NULL);
    }

    ioctl(s_watchdog_fd, WDIOC_SETTIMEOUT, &timeout);

     if(0 != reset_timer(watch_dog_timer))
     {
    watch_dog_timer = create_timer(app_watchdog_feed, WATCHDOG_TIMEOUT*1000, NULL, TIMER_REPEAT);
     }

    return GXCORE_SUCCESS;
}
status_t app_watchdog_close(void)
{
    if(s_watchdog_fd != -1)
    {
        //app_log_debug("[%s][%d] close watchdog !!!\n", __func__, __LINE__);
        ioctl(s_watchdog_fd, WDIOS_DISABLECARD, NULL);
        close(s_watchdog_fd);
        s_watchdog_fd = -1;
    }

    return GXCORE_SUCCESS;
}

#endif

