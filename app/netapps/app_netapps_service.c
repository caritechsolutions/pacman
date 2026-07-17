#include "app.h"
#if PLUGINSTORE_SUPPORT || PLUGINONLINE_SUPPORT
#include "app_netapps_service.h"

typedef struct
{
	int service_running;
	int service_busy;
	int service_mutex;
	app_netapps_service_func func;
}NetAppsService;

static NetAppsService apps_service;

static void netapps_service_thread(void* arg)
{
	GxCore_ThreadDetach();
	while(apps_service.service_running)
	{
		GxCore_MutexLock(apps_service.service_mutex);
		if((apps_service.service_busy == 0)&&apps_service.func)
		{
			apps_service.service_busy = 1;
			GxCore_MutexUnlock(apps_service.service_mutex);
			apps_service.func();
			GxCore_MutexLock(apps_service.service_mutex);
			apps_service.func = NULL;
			apps_service.service_busy = 0;
			GxCore_MutexUnlock(apps_service.service_mutex);
		}
		else
		{
			GxCore_MutexUnlock(apps_service.service_mutex);
			GxCore_ThreadDelay(10);
		}
	}
}

void app_netapps_service_init(void)
{
	static int thread_id = 0;

	memset(&apps_service, 0, sizeof(NetAppsService));
	apps_service.service_running = 1;
	GxCore_MutexCreate(&(apps_service.service_mutex));
	GxCore_ThreadCreate("apps_service", &thread_id, netapps_service_thread,
		NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

void app_netapps_service_destroy(void)
{
	apps_service.service_running = 0;
	GxCore_MutexDelete(apps_service.service_mutex);
	apps_service.service_mutex = -1;
}

int app_netapps_service_push(app_netapps_service_func func)
{
	int ret = -1;

	GxCore_MutexLock(apps_service.service_mutex);
	if((apps_service.service_busy==0)&&(!apps_service.func)&&(func))
	{
		apps_service.func = func;
		ret = 0;
	}
	GxCore_MutexUnlock(apps_service.service_mutex);
	return ret;
}
#endif
