#include "MultiThreading.h"
#include "gxos/gxcore_os.h"

THREAD_HANDLE CreateThread(void (*start_routine) (void *), void *arg)
{
	int ret;
	handle_t thread_handle = -1;

	ret = GxCore_ThreadCreate("MultiThreading", &thread_handle, start_routine, arg, 32*1024, GXOS_DEFAULT_PRIORITY);
	if (ret != GXCORE_SUCCESS)
		return -1;

	return thread_handle;
}
